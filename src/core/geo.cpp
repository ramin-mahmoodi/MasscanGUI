#include "geo.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace masscan::geo {

namespace {

struct Database {
    std::vector<Country> countries;
    std::vector<uint8_t> raw_blob;
    size_t blob_offset = 0;
    size_t blob_length = 0;
};

Database g_db;

inline uint32_t u32_le(const uint8_t* b) {
    return static_cast<uint32_t>(b[0]) |
          (static_cast<uint32_t>(b[1]) << 8) |
          (static_cast<uint32_t>(b[2]) << 16) |
          (static_cast<uint32_t>(b[3]) << 24);
}

inline uint16_t u16_le(const uint8_t* b) {
    return static_cast<uint16_t>(b[0]) |
          (static_cast<uint16_t>(b[1]) << 8);
}

uint32_t read_varint(const uint8_t* b, size_t& i) {
    uint32_t shift = 0;
    uint32_t v = 0;
    for (;;) {
        const uint8_t byte = b[i++];
        v |= static_cast<uint32_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            return v;
        }
        shift += 7;
    }
}

} // anonymous namespace

bool init(std::span<const uint8_t> b) {
    if (b.size() < 7 || b[0] != 'M' || b[1] != 'G' || b[2] != 'E' || b[3] != 'O' || b[4] != '1') {
        return false;
    }

    const size_t n = u16_le(&b[5]);
    size_t i = 7;
    std::vector<Country> list;
    list.reserve(n);

    for (size_t k = 0; k < n; ++k) {
        if (i + 3 > b.size()) return false;
        std::string code(reinterpret_cast<const char*>(&b[i]), 2);
        const size_t nl = b[i + 2];
        i += 3;
        if (i + nl + 16 > b.size()) return false;
        std::string name(reinterpret_cast<const char*>(&b[i]), nl);
        i += nl;

        Country c;
        c.code = std::move(code);
        c.name = std::move(name);
        c.ranges = u32_le(&b[i]);
        c.addresses = u32_le(&b[i + 4]);
        c.offset = u32_le(&b[i + 8]);
        c.length = u32_le(&b[i + 12]);
        i += 16;
        list.push_back(std::move(c));
    }

    if (i + 4 > b.size()) return false;
    const size_t blen = u32_le(&b[i]);
    i += 4;
    if (i + blen > b.size()) return false;

    g_db.countries = std::move(list);
    g_db.raw_blob.assign(b.begin(), b.end());
    g_db.blob_offset = i;
    g_db.blob_length = blen;

    return true;
}

bool init_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    const auto size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }
    return init(buffer);
}

const std::vector<Country>& countries() {
    return g_db.countries;
}

const Country* find(std::string_view code) {
    for (const auto& c : g_db.countries) {
        if (c.code.size() == code.size() &&
            std::equal(c.code.begin(), c.code.end(), code.begin(),
                       [](char a, char b) {
                           return std::tolower(static_cast<unsigned char>(a)) ==
                                  std::tolower(static_cast<unsigned char>(b));
                       })) {
            return &c;
        }
    }
    return nullptr;
}

std::vector<std::pair<uint32_t, uint8_t>> decode_ranges(const Country& country) {
    std::vector<std::pair<uint32_t, uint8_t>> out;
    out.reserve(country.ranges);

    const size_t start = g_db.blob_offset + country.offset;
    const size_t end = start + country.length;
    if (end > g_db.raw_blob.size()) {
        return out;
    }

    const uint8_t* d = g_db.raw_blob.data();
    size_t i = start;
    uint32_t prev = 0;

    while (i < end) {
        prev += read_varint(d, i);
        const uint8_t prefix = d[i++];
        out.emplace_back(prev, prefix);
    }

    return out;
}

std::vector<std::string> decode_cidrs(const Country& country) {
    const auto ranges = decode_ranges(country);
    std::vector<std::string> result;
    result.reserve(ranges.size());
    for (const auto& [ip, prefix] : ranges) {
        result.push_back(ip_to_string(ip) + "/" + std::to_string(prefix));
    }
    return result;
}

std::string ip_to_string(uint32_t ip) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                  (ip >> 24) & 0xFF,
                  (ip >> 16) & 0xFF,
                  (ip >> 8) & 0xFF,
                  ip & 0xFF);
    return std::string(buf);
}

std::string human_number(uint64_t n) {
    char buf[32];
    if (n >= 1'000'000'000ULL) {
        std::snprintf(buf, sizeof(buf), "%.2fB", static_cast<double>(n) / 1e9);
    } else if (n >= 1'000'000ULL) {
        std::snprintf(buf, sizeof(buf), "%.1fM", static_cast<double>(n) / 1e6);
    } else if (n >= 1'000ULL) {
        std::snprintf(buf, sizeof(buf), "%.1fK", static_cast<double>(n) / 1e3);
    } else {
        std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(n));
    }
    return std::string(buf);
}

} // namespace masscan::geo
