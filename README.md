# MasscanGUI

A high-performance, modern C++20 / Qt 6 graphical user interface for [masscan](https://github.com/robertdavidgraham/masscan), the world's fastest Internet-scale port scanner.

Designed with Microsoft Fluent design principles, animated transitions, light/dark themes, and real-time packet parsing.

---

## ✨ Features

- **Blazing Fast & Low Overhead**: Native C++20 implementation built on Qt 6 with zero garbage collection overhead.
- **Adaptive Responsive Layout**: Fluid 2-column interface on widescreen displays with smooth overlay drawer on compact screens (<880px).
- **Interactive Morphing Theme Switch**: Custom circular clip shader animation transitioning seamlessly between dark and light themes at 120/144Hz.
- **Country IP Target Selector**: Integrated offline GeoIP database (`geo.bin`) allowing single-click targeting of entire countries and CIDR ranges.
- **Live Output Tailer & Multi-Format Parsing**:
  - Live status tracking (kpps rate, % completed, ETA, found count) streaming on carriage return (`\r`) chunks.
  - Native support for standard stdout, list format (`-oL`), NDJSON (`-oD`), and JSON (`-oJ`).
  - Automatic port-to-service mapping (HTTP, HTTPS, SSH, FTP, DNS, MySQL, RDP, Redis, Postgres, MongoDB, etc.).
- **Memory & UI Protection**:
  - Duplicate SYN-ACK packet elimination.
  - Automatic GUI table row capping to prevent thread freezing on internet-scale scans.
- **Dependency Health Indicators**: Real-time header badges detecting presence and status of both `masscan` and `Npcap`/`WinPcap`.

---

## 🛠 Prerequisites

- **Compiler**: Modern C++20 compiler (MSVC 2022 v17+, GCC 11+, or Clang 13+)
- **Build System**: CMake 3.21 or higher
- **Framework**: Qt 6.5+ (Qt 6.8+ recommended) with modules:
  - `Qt6::Core`
  - `Qt6::Gui`
  - `Qt6::Widgets`
  - `Qt6::Network`
  - `Qt6::Svg`
- **Driver (Windows)**: [Npcap](https://npcap.com/) for raw packet transmission.

---

## 🚀 Building from Source

### Windows (MSVC & CMake)

```powershell
# 1. Clone repository
git clone https://github.com/ramin-mahmoodi/MasscanGUI.git
cd MasscanGUI

# 2. Configure build with Qt 6
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3. Build executable
cmake --build build --config Release
```

The compiled binary will be placed at `build/Release/MasscanGUI.exe`.

---

## 📄 License

This project is licensed under the MIT License.
Masscan is developed by Robert David Graham.
