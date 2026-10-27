#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace masscan {

enum class WarnKind {
    Info,
    Warn,
    Err,
};

struct Warning {
    WarnKind kind;
    std::string text;
};

struct Snapshot {
    std::string exclude;
    std::string excludefile;
    std::string ports{"80,443,8000-8100"};
    std::string top;
    std::string exports;
    bool tcp{true};
    bool udp{false};
    bool sctp{false};
    bool oproto{false};
    bool ping{false};
    bool arp{false};

    std::string rate{"2500"};
    std::string retries;
    std::string wait;
    std::string seed;
    std::string shard;
    std::string ttl;
    bool offline{false};
    std::string extra;

    std::string adapter;
    std::string aip;
    std::string aport;
    std::string amac;
    std::string rmac;
    std::string vlan;

    bool banners{false};
    bool cap_cert{false};
    bool cap_servername{false};
    bool cap_html{false};
    bool cap_heartbleed{false};
    bool cap_ticketbleed{false};

    std::string ua;
    std::string hello;
    std::string ctimeout;
    std::string nmappl;
    std::string pcappl;

    std::string fmt{"D"};
    std::string outfile{"scan.ndjson"};
    bool open_only{true};
    bool flush{true};
    bool append{false};
    bool pktrace{false};
    std::string rotate;
    std::string rotdir;
    std::string pcap;

    std::string outdir{"MassGUI"};
    std::string bin{"masscan"};
    std::string mss;
    std::string minpkt;
    std::string bpf;

    bool all_ipv4{false};
    int mode{0};
    int verbose{0};
    std::string conffile;
    bool infinite{false};
    bool sendq{false};
    std::string blackrock;
    bool noreset{false};
    std::string rtip;
    bool badsum{false};
    std::string datalen;
    std::string wscale;
    std::string tsecho;
    bool sackok{false};
    bool rawudp{false};
    std::string hello_kind;
    std::string hello_file;
    std::string hello_timeout;
    std::string http_method;
    std::string http_url;
    std::string http_version;
    std::string http_host;
    std::string http_cookie;
    std::string http_header;
    std::string http_payload;
    std::string vuln;
    std::string nocap;
    std::string script;
    std::string probes;
    std::string datadir;
    std::string rot_offset;
    std::string rot_size;
    std::string stylesheet;
    bool no_stylesheet{false};
    bool reason{false};
    std::string show;
    std::string noshow;
    bool interactive{false};
    bool nostatus{false};
    bool ndjson_status{false};
    bool json_status{false};
    std::string rmac6;
};

struct BuiltCommand {
    std::vector<std::string> args;
    std::vector<Warning> warnings;
    uint64_t target_ips{0};
    uint64_t port_count{0};
    uint64_t total_probes{0};
    uint64_t rate_num{0};
    double est_seconds{0.0};
};

std::string thousands(uint64_t n);
std::string fmt_duration(double secs);
std::optional<uint64_t> target_size(std::string_view text);
std::optional<uint64_t> count_ports(std::string_view spec, std::string_view top);

bool write_targets_file(const std::string& path,
                        const std::vector<std::string>& manual,
                        const std::vector<std::string>& countries,
                        size_t& out_lines,
                        uint64_t& out_addresses);

BuiltCommand build_command(const Snapshot& s,
                           const std::vector<std::string>& manual,
                           const std::vector<std::string>& countries,
                           bool resume = false);

} // namespace masscan
