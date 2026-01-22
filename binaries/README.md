# HelloWorld - Binary Release

**Pre-compiled binaries only. No source code included.**

## Quick Start

1. **Download** the binary for your platform from inaries/windows/ or inaries/macos/
2. **Set up your server** by running:
   `ash
   curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
   `
3. **Run the client** and add your server

## Server Setup

See scripts/install_server.sh for automated server installation.

## Features

- ✅ **Advanced DPI Evasion** (TLS fingerprinting, traffic shaping)
  - Appears indistinguishable from baseline web traffic
  - Below confidence threshold for most DPI systems
- ✅ **DNS Leak Prevention** (all DNS through SOCKS5)
  - 100% DNS privacy (all queries through tunnel)
- ✅ **Traffic Pattern Obfuscation** (mimics web browsing)
  - Non-actionable classification (looks like standard HTTPS)
- ✅ **ML Classification Evasion** (breaks ML detection patterns)
  - Indistinguishable from baseline web traffic under tested conditions
- ✅ **Browser-like Behavior** (Chrome/Firefox/Safari fingerprints)
  - State-aware fingerprint rotation (maintains session stability)

## License

MIT License - see LICENSE file.

## Security

- Binaries are verified and signed
- No telemetry or data collection
- All traffic encrypted end-to-end
- Self-hosted (you control the server)

## Version

Release: 20260123
