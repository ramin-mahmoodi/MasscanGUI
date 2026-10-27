#include "command_builder.hpp"
#include "geo.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace masscan {

namespace {

bool parse_ipv4(std::string_view s, uint32_t& out_ip) {
    unsigned int a, b, c, d;
    char dummy;
    std::string str(s);
    if (std::sscanf(str.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &dummy) == 4) {
        if (a <= 255 && b <= 255 && c <= 255 && d <= 255) {
            out_ip = (a << 24) | (b << 16) | (c << 8) | d;
            return true;
        }
    }
    return false;
}

std::pair<uint64_t, bool> total_hosts(const std::vector<std::string>& manual,
                                      const std::vector<std::string>& codes) {
    uint64_t sum = 0;
    bool exact = true;
    for (const auto& m : manual) {
        if (auto sz = target_size(m)) {
            sum += *sz;
        } else {
            exact = false;
        }
    }
    for (const auto& c : codes) {
        if (const auto* cn = geo::find(c)) {
            sum += cn->addresses;
        }
    }
    return {sum, exact};
}

} // anonymous namespace

std::string thousands(uint64_t n) {
    std::string s = std::to_string(n);
    std::string out;
    const size_t len = s.length();
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && (len - i) % 3 == 0) {
            out.push_back(',');
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string fmt_duration(double secs) {
    uint64_t s = static_cast<uint64_t>(std::max(0.0, std::round(secs)));
    const uint64_t d = s / 86400;
    const uint64_t h = (s % 86400) / 3600;
    const uint64_t m = (s % 3600) / 60;
    const uint64_t x = s % 60;

    char buf[64];
    if (d > 0) {
        std::snprintf(buf, sizeof(buf), "%llud %lluh", static_cast<unsigned long long>(d), static_cast<unsigned long long>(h));
    } else if (h > 0) {
        std::snprintf(buf, sizeof(buf), "%lluh %llum", static_cast<unsigned long long>(h), static_cast<unsigned long long>(m));
    } else if (m > 0) {
        std::snprintf(buf, sizeof(buf), "%llum %llus", static_cast<unsigned long long>(m), static_cast<unsigned long long>(x));
    } else {
        std::snprintf(buf, sizeof(buf), "%llus", static_cast<unsigned long long>(x));
    }
    return std::string(buf);
}

std::optional<uint64_t> target_size(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    if (text.empty()) return std::nullopt;

    if (auto slash = text.find('/'); slash != std::string_view::npos) {
        uint32_t ip = 0;
        if (!parse_ipv4(text.substr(0, slash), ip)) return std::nullopt;
        uint32_t bits = 0;
        auto res = std::from_chars(text.data() + slash + 1, text.data() + text.size(), bits);
        if (res.ec == std::errc{} && bits <= 32) {
            return (1ULL << (32 - bits));
        }
        return std::nullopt;
    }

    if (auto dash = text.find('-'); dash != std::string_view::npos) {
        uint32_t a = 0, b = 0;
        if (!parse_ipv4(text.substr(0, dash), a) || !parse_ipv4(text.substr(dash + 1), b)) {
            return std::nullopt;
        }
        if (b >= a) {
            return static_cast<uint64_t>(b - a + 1);
        }
        return std::nullopt;
    }

    uint32_t ip = 0;
    if (parse_ipv4(text, ip)) {
        return 1ULL;
    }
    return std::nullopt;
}

std::optional<uint64_t> count_ports(std::string_view spec, std::string_view top) {
    uint64_t total = 0;
    size_t start = 0;
    while (start < spec.size()) {
        auto comma = spec.find(',', start);
        std::string_view tok = spec.substr(start, comma == std::string_view::npos ? spec.size() - start : comma - start);
        start = (comma == std::string_view::npos) ? spec.size() : comma + 1;

        while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.front()))) tok.remove_prefix(1);
        while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.back()))) tok.remove_suffix(1);
        if (tok.empty()) continue;

        if (auto colon = tok.find(':'); colon != std::string_view::npos) {
            if (colon == 1 && std::isalpha(static_cast<unsigned char>(tok[0]))) {
                tok = tok.substr(2);
            } else {
                return std::nullopt;
            }
        }

        uint32_t a = 0, b = 0;
        if (auto dash = tok.find('-'); dash != std::string_view::npos) {
            auto r1 = std::from_chars(tok.data(), tok.data() + dash, a);
            auto r2 = std::from_chars(tok.data() + dash + 1, tok.data() + tok.size(), b);
            if (r1.ec != std::errc{} || r2.ec != std::errc{}) return std::nullopt;
        } else {
            auto r = std::from_chars(tok.data(), tok.data() + tok.size(), a);
            if (r.ec != std::errc{}) return std::nullopt;
            b = a;
        }

        if (a > 65535 || b > 65535 || b < a) return std::nullopt;
        total += (b - a + 1);
    }

    while (!top.empty() && std::isspace(static_cast<unsigned char>(top.front()))) top.remove_prefix(1);
    while (!top.empty() && std::isspace(static_cast<unsigned char>(top.back()))) top.remove_suffix(1);
    if (!top.empty()) {
        uint64_t t = 0;
        auto res = std::from_chars(top.data(), top.data() + top.size(), t);
        if (res.ec == std::errc{}) {
            total += t;
        } else {
            return std::nullopt;
        }
    }

    return total;
}

bool write_targets_file(const std::string& path,
                        const std::vector<std::string>& manual,
                        const std::vector<std::string>& countries,
                        size_t& out_lines,
                        uint64_t& out_addresses) {
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream f(path);
        if (!f.is_open()) return false;

        f << "# generated by Masscan GUI\n";
        out_lines = 0;
        out_addresses = 0;

        for (const auto& m : manual) {
            f << m << "\n";
            out_lines++;
            if (auto sz = target_size(m)) {
                out_addresses += *sz;
            }
        }

        for (const auto& code : countries) {
            if (const auto* c = geo::find(code)) {
                f << "# " << c->name << " (" << c->code << ")\n";
                auto cidrs = geo::decode_cidrs(*c);
                for (const auto& cidr : cidrs) {
                    f << cidr << "\n";
                    out_lines++;
                }
                out_addresses += c->addresses;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

BuiltCommand build_command(const Snapshot& c, const std::vector<std::string>& manual,
                           const std::vector<std::string>& countries, bool resume) {
    BuiltCommand out;

    if (resume) {
        out.args = {"masscan", "--resume", "paused.conf"};
        return out;
    }

    std::string bin = c.bin.empty() ? "masscan" : c.bin;
    if (c.mode == 3) {
        out.args = {bin, "--iflist"};
        return out;
    }
    if (c.mode == 4) {
        out.args = {bin, "--selftest"};
        return out;
    }

    out.args.push_back(bin);
    if (c.verbose >= 1) {
        out.args.push_back(c.verbose >= 2 ? "-vv" : "-v");
    }
    if (!c.conffile.empty()) {
        out.args.push_back("-c");
        out.args.push_back(c.conffile);
    }

    auto add = [&](const char* flag, const std::string& val) {
        if (!val.empty()) {
            out.args.push_back(flag);
            out.args.push_back(val);
        }
    };

    auto quote = [](const std::string& s) -> std::string {
        if (s.find(' ') == std::string::npos) return s;
        std::string q = "\"";
        for (char ch : s) {
            if (ch == '"') q += "\\\"";
            else q += ch;
        }
        q += "\"";
        return q;
    };

    // targets
    std::vector<std::string> effective_manual = manual;
    if (c.all_ipv4 && std::find(effective_manual.begin(), effective_manual.end(), "0.0.0.0/0") == effective_manual.end()) {
        effective_manual.insert(effective_manual.begin(), "0.0.0.0/0");
    }
    if (c.all_ipv4 && c.exclude.empty() && c.excludefile.empty()) {
        out.warnings.push_back({
            WarnKind::Warn,
            "Masscan requires at least one --exclude (e.g. 255.255.255.255) as confirmation when scanning 0.0.0.0/0."
        });
    }

    if (!countries.empty()) {
        std::string dir = c.outdir.empty() ? "." : c.outdir;
        out.args.push_back("-iL");
        out.args.push_back(dir + "/targets.txt");

        uint64_t ranges = 0, ips = 0;
        for (const auto& code : countries) {
            if (const auto* cn = geo::find(code)) {
                ranges += cn->ranges;
                ips += cn->addresses;
            }
        }
        std::string extra = effective_manual.empty() ? "" : " + " + std::to_string(effective_manual.size()) + " target(s)";
        out.warnings.push_back({
            WarnKind::Info,
            std::to_string(countries.size()) + " countr" + (countries.size() == 1 ? "y" : "ies") + extra +
            ": " + thousands(ranges) + " ranges, " + geo::human_number(ips) + " IPs. The app writes them to " +
            dir + "/targets.txt when you press Start."
        });
    } else if (!effective_manual.empty()) {
        std::string combined;
        for (size_t i = 0; i < effective_manual.size(); ++i) {
            if (i > 0) combined += ",";
            combined += effective_manual[i];
        }
        out.args.push_back(combined);
    } else if (!c.all_ipv4 && c.conffile.empty()) {
        out.warnings.push_back({WarnKind::Err, "No target: add a country or an IP/range."});
    }

    add("--exclude", c.exclude);
    add("--excludefile", c.excludefile);

    // ports
    add("-p", c.ports);
    add("--top-ports", c.top);
    add("--exclude-ports", c.exports);
    if (c.ports.empty() && c.top.empty()) {
        out.warnings.push_back({WarnKind::Err, "No ports: masscan has no default ports. Set -p or --top-ports."});
    }
    bool valid_port_chars = std::all_of(c.ports.begin(), c.ports.end(), [](unsigned char ch) {
        return std::isdigit(ch) || std::string_view("UuSs:,- ").find(static_cast<char>(ch)) != std::string_view::npos;
    });
    if (!valid_port_chars) {
        out.warnings.push_back({WarnKind::Err, "Ports: invalid characters."});
    }

    // scan types (none selected -> masscan defaults to TCP)
    if (c.udp) out.args.push_back("-sU");
    if (c.sctp) out.args.push_back("-sZ");
    if (c.oproto) out.args.push_back("-sO");
    if (c.tcp && (c.udp || c.sctp || c.oproto)) out.args.push_back("-sS");
    if (c.ping) out.args.push_back("--ping");
    if (c.arp) out.args.push_back("--arp");
    if (c.udp && !c.ports.empty()) {
        std::string upper_ports = c.ports;
        std::transform(upper_ports.begin(), upper_ports.end(), upper_ports.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        if (upper_ports.find("U:") == std::string::npos) {
            out.warnings.push_back({WarnKind::Warn, "UDP selected but no U: ports given (e.g. U:53)."});
        }
    }

    // rate & timing
    add("--rate", c.rate);
    add("--retries", c.retries);
    add("--wait", c.wait);
    add("--seed", c.seed);
    add("--shard", c.shard);
    add("--ttl", c.ttl);
    if (c.offline) out.args.push_back("--offline");
    if (c.infinite) out.args.push_back("--infinite");
    if (c.sendq) out.args.push_back("--sendq");
    if (c.noreset) out.args.push_back("--noreset");
    add("--blackrock-rounds", c.blackrock);

    uint64_t shards = 1;
    if (!c.shard.empty()) {
        bool valid_shard = false;
        if (auto slash = c.shard.find('/'); slash != std::string::npos) {
            uint32_t n = 0, t = 0;
            const char* first_end = c.shard.data() + slash;
            const char* second_start = first_end + 1;
            const char* second_end = c.shard.data() + c.shard.size();
            auto r1 = std::from_chars(c.shard.data(), first_end, n);
            auto r2 = std::from_chars(second_start, second_end, t);
            if (r1.ec == std::errc{} && r1.ptr == first_end &&
                r2.ec == std::errc{} && r2.ptr == second_end &&
                n >= 1 && n <= t) {
                valid_shard = true;
                shards = t;
                if (t > 1 && c.seed.empty()) {
                    out.warnings.push_back({WarnKind::Warn, "All shards must share the same --seed. Set one."});
                }
            }
        }
        if (!valid_shard) {
            out.warnings.push_back({WarnKind::Err, "Shard must look like 1/3 (index from 1, not above total)."});
        }
    }

    // network
    add("-e", c.adapter);
    add("--adapter-ip", c.aip);
    add("--adapter-port", c.aport);
    add("--adapter-mac", c.amac);
    add("--router-mac", c.rmac);
    add("--router-ip", c.rtip);
    add("--router-mac-ipv6", c.rmac6);
    add("--vlan", c.vlan);

    // banners
    if (c.banners) out.args.push_back("--banners");
    if (c.cap_cert) { out.args.push_back("--capture"); out.args.push_back("cert"); }
    if (c.cap_servername) { out.args.push_back("--capture"); out.args.push_back("servername"); }
    if (c.cap_html) { out.args.push_back("--capture"); out.args.push_back("html"); }
    if (c.cap_heartbleed) { out.args.push_back("--capture"); out.args.push_back("heartbleed"); }
    if (c.cap_ticketbleed) { out.args.push_back("--capture"); out.args.push_back("ticketbleed"); }
    if (c.rawudp) out.args.push_back("--rawudp");
    add("--hello", c.hello_kind);
    add("--vuln", c.vuln);
    add("--nocapture", c.nocap);
    add("--hello-file", c.hello_file);
    add("--hello-timeout", c.hello_timeout);
    const std::pair<const char*, const std::string*> http_opts[] = {
        {"--http-method", &c.http_method},
        {"--http-url", &c.http_url},
        {"--http-version", &c.http_version},
        {"--http-host", &c.http_host},
        {"--http-cookie", &c.http_cookie},
        {"--http-header", &c.http_header},
        {"--http-payload", &c.http_payload},
    };
    for (const auto& [flag, val] : http_opts) {
        if (!val->empty()) {
            out.args.push_back(flag);
            out.args.push_back(quote(*val));
        }
    }
    if (!c.ua.empty()) {
        out.args.push_back("--http-user-agent");
        out.args.push_back(quote(c.ua));
    }
    if (!c.hello.empty()) {
        out.args.push_back("--hello-string");
        out.args.push_back(quote(c.hello));
    }
    add("--connection-timeout", c.ctimeout);
    add("--nmap-payloads", c.nmappl);
    add("--pcap-payloads", c.pcappl);

    // output
    if (!c.outfile.empty()) {
        out.args.push_back("-o" + c.fmt);
        if (c.fmt == "R" || c.outdir.empty()) {
            out.args.push_back(c.outfile);
        } else {
            out.args.push_back(c.outdir + "/" + c.outfile);
        }
    } else {
        out.warnings.push_back({WarnKind::Warn, "No output file: results only go to the console/log."});
    }

    if (c.open_only) out.args.push_back("--open");
    if (c.flush) out.args.push_back("--output-flush");
    if (c.append) out.args.push_back("--append-output");
    if (c.pktrace) out.args.push_back("--packet-trace");
    add("--rotate", c.rotate);
    add("--rotate-dir", c.rotdir);
    add("--rotate-offset", c.rot_offset);
    add("--rotate-size", c.rot_size);
    add("--show", c.show);
    add("--noshow", c.noshow);
    add("--stylesheet", c.stylesheet);
    if (c.no_stylesheet) out.args.push_back("--no-stylesheet");
    if (c.reason) out.args.push_back("--reason");
    if (c.interactive) out.args.push_back("--interactive");
    if (c.nostatus) out.args.push_back("--nostatus");
    if (c.ndjson_status) out.args.push_back("--ndjson-status");
    if (c.json_status) out.args.push_back("--json-status");
    add("--pcap", c.pcap);

    // advanced
    add("--tcp-mss", c.mss);
    add("--min-packet", c.minpkt);
    add("--bpf", c.bpf);
    if (c.badsum) out.args.push_back("--badsum");
    add("--data-length", c.datalen);
    add("--tcp-wscale", c.wscale);
    add("--tcp-tsecho", c.tsecho);
    if (c.sackok) out.args.push_back("--tcp-sackok");
    add("--datadir", c.datadir);
    add("--nmap-service-probes", c.probes);
    add("--script", c.script);
    if (!c.datalen.empty()) {
        uint32_t dl = 0;
        const char* dl_end = c.datalen.data() + c.datalen.size();
        auto r = std::from_chars(c.datalen.data(), dl_end, dl);
        if (r.ec != std::errc{} || r.ptr != dl_end || dl >= 1460) {
            out.warnings.push_back({WarnKind::Err, "Data length must be a number below 1460 (masscan rejects larger values)."});
        }
    }
    if (c.mode == 1) out.args.push_back("-sL");
    if (c.mode == 2) out.args.push_back("--echo");
    if (!c.extra.empty()) out.args.push_back(c.extra);

    // Time estimate (masscan's own default rate is 100 pps)
    std::optional<double> rate_opt;
    if (c.rate.empty()) {
        rate_opt = 100.0;
    } else {
        char* endptr = nullptr;
        double parsed = std::strtod(c.rate.c_str(), &endptr);
        if (endptr == c.rate.c_str() + c.rate.size() && parsed > 0.0) {
            rate_opt = parsed;
        }
    }
    auto [hosts, exact] = total_hosts(manual, countries);
    if (c.all_ipv4) {
        hosts += (1ULL << 32);
        exact = false;
    }
    auto ports = count_ports(c.ports, c.top);
    if (rate_opt && ports) {
        if (hosts > 0 && *ports > 0) {
            bool has_err = false;
            for (const auto& w : out.warnings) {
                if (w.kind == WarnKind::Err) { has_err = true; break; }
            }
            if (!has_err) {
                double secs = (static_cast<double>(hosts) * static_cast<double>(*ports)) /
                              (static_cast<double>(std::max(1ULL, shards)) * (*rate_opt));
                out.est_seconds = secs;
                out.warnings.push_back({
                    WarnKind::Info,
                    "Estimate: " + std::string(exact ? "" : "≥ ") + geo::human_number(hosts) + " IPs × " +
                    thousands(*ports) + " port(s) at " + thousands(static_cast<uint64_t>(*rate_opt)) + " pps ≈ " +
                    fmt_duration(secs) + (shards > 1 ? " per shard (1/" + std::to_string(shards) + ")" : "") + "."
                });
            }
        }
    }

    return out;
}

} // namespace masscan
