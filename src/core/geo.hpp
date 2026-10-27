#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <span>

namespace masscan::geo {

struct Country {
    std::string code;       // lowercase ISO 3166-1 alpha-2, e.g. "ir"
    std::string name;       // "Iran"
    uint32_t ranges = 0;    // number of CIDR ranges
    uint32_t addresses = 0; // total IPv4 addresses covered
    uint32_t offset = 0;    // byte offset in blob
    uint32_t length = 0;    // byte length in blob
};

// Initializes the database from an in-memory buffer (e.g. QByteArray / span)
bool init(std::span<const uint8_t> buffer);

// Initializes by reading a file path on disk
bool init_from_file(const std::string& path);

// Returns all loaded countries (sorted by code)
const std::vector<Country>& countries();

// Lookup country by 2-letter ISO code (case-insensitive)
const Country* find(std::string_view code);

// Decodes the (start_ip, prefix_len) pairs for a country on demand
std::vector<std::pair<uint32_t, uint8_t>> decode_ranges(const Country& country);

// Decodes CIDR strings (e.g. "2.176.0.0/12") for a country on demand
std::vector<std::string> decode_cidrs(const Country& country);

// Formats a 32-bit IPv4 address into "A.B.C.D" string
std::string ip_to_string(uint32_t ip);

// Human readable number formatter: 12_345_678 -> "12.3M", 4_096 -> "4.1K"
std::string human_number(uint64_t n);

} // namespace masscan::geo
