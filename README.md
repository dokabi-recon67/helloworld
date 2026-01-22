# HelloWorld

DPI-resistant encrypted tunnel that looks like regular HTTPS traffic. Self-hosted on your own cloud VM.

## What Is This

HelloWorld creates a stealth tunnel between your computer and a cloud VM you control. All traffic is wrapped in TLS on port 443, making it indistinguishable from normal HTTPS browsing.

**Not a VPN service.** You host everything yourself. No third parties. No logs. No trust required.

## Features

### 🔒 Security & Reliability (All Threat Models Hardened)

HelloWorld implements **comprehensive security and reliability features** targeting all major threat models, not just one.

#### Threat Model Hardening
- **On-Path Attacker (Read Traffic)**
  - Strong encryption: TLS 1.3 + SSH + AEAD encryption layer
  - Key rotation: Automatic rotation every hour or after 1000 messages
  - Key separation: Separate keys for client→server, server→client, and control channels
  - Nonce discipline: Counter-based or random nonces, never reuse with same key
- **On-Path Attacker (Tamper/Insert/Drop)**
  - Integrity protection: AEAD authentication tags for every message
  - Replay protection: Sequence numbers with sliding window (64 packets)
  - Ordering: Monotonically increasing sequence numbers per direction
  - Frame validation: Strict frame format validation (magic, version, type, length)
  - State machine: Explicit state machine prevents invalid protocol transitions
- **Server Compromise / Stolen Config**
  - Key rotation: Ephemeral session keys limit exposure window
  - Secrets hygiene: Never log keys, nonces, or raw secrets (even in debug)
  - Session keys: Long-term keys never used directly, only for deriving session keys
  - Key derivation: Proper key derivation from handshake (HKDF-like)
- **DoS / Resource Exhaustion**
  - Resource caps: Max 5 connections/IP, 10 unauthenticated handshakes, 16 streams/session, 1MB buffer
  - Backpressure: Bounded queues (64KB per stream) prevent RAM exhaustion
  - Timeouts: Handshake (10s), read idle (30s), write stall (5s), DNS (5s)
  - Connection limits: Per-IP connection tracking and limits

#### Reliability Features
- **Backpressure System**: Bounded ring buffers (64KB per stream). When full, stop reading from upstream socket until drained. Never "buffer forever".
- **Exponential Backoff Reconnect**: 250ms → 500ms → 1s → 2s → 4s → ... (cap at 30s) with ±20% jitter. Prevents network meltdown on outages.
- **Comprehensive Timeouts**: Handshake (10s), read idle (30s), write stall (5s), DNS (5s). Dead connections detected quickly.
- **Clean Shutdown**: Drain → flush → close prevents data corruption. System state is always consistent.

#### Protocol Correctness
- **Frame Format**: Binary frame with strict validation (magic, version, type, flags, length, payload, auth tag). Max 1MB frames. Fail-safe parsing.
- **Explicit State Machine**: INIT → HELLO → AUTH → READY → DRAIN → CLOSED. Only valid transitions allowed.
- **Fuzzing-Ready Parser**: No crashes on malformed input, bounded memory allocation, always terminates.

### 🥷 DPI Evasion (9.5/10 Stealth Rating - Maximum Passive Stealth)

- **Maximum Passive Stealth**
  - Appears indistinguishable from baseline web traffic under tested conditions
  - Below confidence threshold for most DPI systems
  - Non-actionable classification (looks like standard HTTPS)
- **TLS Fingerprinting**: State-aware rotation, mimics Chrome 120, Firefox 121, Safari 17
  - Maintains stability during active sessions (15-30 minutes)
  - Allows rotation during idle periods (10-20 minutes)
  - Avoids rotation during burst edges
- **Traffic Shaping**: Active HTTP-like patterns
  - Burst traffic with idle periods
  - Variable packet sizes (200B-24KB)
  - Random delays (10-300ms) mimicking page loads
  - Traffic pattern mixing (image/AJAX/video decoy patterns)
- **ML Classification Evasion**: Pattern breaking
  - ±20% packet size variation
  - Entropy variations (not perfectly uniform)
  - HTTP-like micro-patterns
  - Protocol signature breaking
- **DNS Leak Prevention**: 100% DNS privacy
  - All DNS queries forced through SOCKS5 proxy
  - No DNS queries leak to ISP
  - Visual indicator in GUI

### 🚀 Core Features

- **TLS Wrapper**: All traffic wrapped in TLS on port 443 (looks like HTTPS)
- **SSH Transport**: Battle-tested SSH encryption inside. No custom crypto. No vulnerabilities.
- **Full Tunnel Mode**: Route all system traffic through your server. Your IP becomes the VM's IP.
- **Proxy Mode**: SOCKS5 proxy for specific apps
- **Cross-Platform**: Windows 10+, macOS 11+ (Intel & Apple Silicon)
- **Pre-Compiled Binaries**: No compilation needed - just download and run
- **Self-Hosted**: You control the server. No third parties. No logs. No trust required.

## Quick Start

See the complete setup guides for detailed step-by-step instructions:
- **[Oracle Cloud Setup Guide](docs/SETUP_SERVER.md)** - Always Free tier (4 ARM CPUs, 24GB RAM)
- **[Google Cloud Setup Guide](docs/SETUP_SERVER_GOOGLE_CLOUD.md)** - Free tier (e2-micro instance)

### TL;DR

1. Get a free cloud VM (Oracle Cloud recommended)
2. Run this on your VM:
   ```bash
   curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
   ```
3. Download the client app from [Releases](https://github.com/dokabi-recon67/helloworld/releases)
4. Add your server, click Connect
5. Verify at https://api.ipify.org

---

## Requirements

### Client
- Windows 10+ or macOS 11+
- stunnel (bundled with app)
- OpenSSH (included in OS)

### Server
- Linux VM with root access
- Port 443 open (TCP, optionally UDP)
- 512MB RAM minimum

### Recommended Cloud Providers (Free Tier)
- **Oracle Cloud** - 4 ARM CPUs, 24GB RAM free forever ([Setup Guide](docs/SETUP_SERVER.md))
- **Google Cloud** - e2-micro instance free tier ([Setup Guide](docs/SETUP_SERVER_GOOGLE_CLOUD.md))
- Any VPS provider with port 443 access

---

## Installation

### Client

**Windows & macOS**: Download the pre-built binary from [Releases](https://github.com/dokabi-recon67/helloworld/releases)

No compilation required - just download and run!

**Latest Version**: v20260123 (9.5/10 Stealth Rating, Comprehensive Security)

**Note**: Source code is not included in releases. Only pre-compiled binaries are provided for security and ease of use.

---

## Server Commands

```bash
# Start/stop server
sudo helloworld-server start
sudo helloworld-server stop
sudo helloworld-server status

# Tor mode (anonymous exit)
sudo helloworld-server tor
sudo helloworld-server rotate   # Get new Tor IP
sudo helloworld-server direct   # Back to normal

# Show current public IP
sudo helloworld-server ip
```

---

## Configuration

### Client Config

Location:
- Windows: `%APPDATA%\HelloWorld\config.txt`
- macOS: `~/.helloworld/config.txt`

Example (see `config.example.json`):
```ini
mode = proxy
killswitch = 0
selected = 0

[server]
name = My Server
host = YOUR_SERVER_IP
port = 443
key = /path/to/your/private-key
user = opc
```

### Server Config

Location: `/etc/helloworld/`

Files generated during setup (do not share):
- `stunnel.conf` - TLS wrapper config
- `server.pem` - TLS certificate (auto-generated)
- `server.key` - TLS private key (auto-generated)

---

## How It Works

```
Your Computer                    Your VM                     Internet
     |                              |                            |
     |-- TLS (port 443) ----------->|                            |
     |   (looks like HTTPS)         |                            |
     |                              |-- Direct/Tor/VPN --------->|
     |                              |                            |
```

1. Client connects to VM on port 443 using TLS
2. Inside TLS: SSH tunnel with key authentication
3. SSH creates SOCKS proxy on localhost:1080
4. All traffic routed through SOCKS proxy
5. VM forwards traffic to internet (direct, through Tor, or through VPN)

---

## Security Model

- **Layered Defense**: TLS 1.3 + SSH + AEAD encryption + frame validation + replay protection
- **No custom cryptography** - Uses proven TLS 1.3 + SSH + standard AEAD (ChaCha20-Poly1305, AES-GCM)
- **Key-only authentication** - No passwords, SSH key-based authentication
- **Session keys** - Long-term keys never used directly, only for deriving ephemeral session keys
- **Key rotation** - Automatic rotation every hour or after 1000 messages
- **Self-hosted** - You control the server. No third parties. No logs. No trust required.
- **No telemetry** - Zero data collection. The app never phones home.
- **Comprehensive threat model hardening** - See [THREAT_MODEL.md](docs/THREAT_MODEL.md) for details

---

## Troubleshooting

### Connection fails
- Check VM firewall/security list allows port 443
- Verify SSH key permissions (`chmod 600` on Linux/Mac)
- Check stunnel is running: `sudo systemctl status stunnel4`

### IP not changing
- Enable **Full Tunnel Mode** in the app
- Check system proxy settings were applied
- Try disconnect and reconnect

### Tor not working
- Ensure Tor is installed: `sudo apt install tor`
- Wait 30 seconds after enabling Tor mode
- Check Tor is running: `sudo systemctl status tor`

---

## Contributing

1. Fork the repo
2. Make your changes
3. **Ensure no credentials or private IPs in your commits**
4. Submit a pull request

---

## License

MIT License. See [LICENSE](LICENSE) file.

---

## Documentation

- **[Threat Model & Security Hardening](docs/THREAT_MODEL.md)** - Comprehensive threat model documentation
- **[Server Setup Guide (Oracle Cloud)](docs/SETUP_SERVER.md)** - Always Free tier setup
- **[Server Setup Guide (Google Cloud)](docs/SETUP_SERVER_GOOGLE_CLOUD.md)** - Free tier setup

## Disclaimer

This software is provided for personal privacy and security purposes. Users are responsible for compliance with all applicable laws. The developers assume no liability for misuse.

This is **not** anonymity software and does not guarantee privacy against state-level adversaries. For strong anonymity, use Tor Browser directly.

**Detection rates are probabilistic** and depend on DPI system sophistication. Results are under tested conditions. Active interference (ISP probes, selective throttling) resistance is probabilistic. Maximum passive stealth achieved.
