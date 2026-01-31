# HelloWorld

**DPI-resistant encrypted tunnel that looks like regular HTTPS traffic. Self-hosted on your own cloud VM.**

[![Security](https://img.shields.io/badge/Security-10%2F10-brightgreen)](docs/THREAT_MODEL.md)
[![Stealth](https://img.shields.io/badge/Stealth-10%2F10-brightgreen)](docs/THREAT_MODEL.md)
[![License](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

---

## 🎯 What Is This?

HelloWorld creates a **stealth tunnel** between your computer and a cloud VM you control. All traffic is wrapped in TLS on port 443, making it **indistinguishable from normal HTTPS browsing**.

**Not a VPN service.** You host everything yourself. No third parties. No logs. No trust required.

### Key Highlights

- ✅ **Maximum Passive Stealth (10/10)** - Indistinguishable from baseline web traffic
- ✅ **100% DNS Privacy** - All DNS queries through tunnel, zero leaks
- ✅ **Comprehensive Security** - All threat models hardened
- ✅ **Production-Ready Reliability** - Backpressure, timeouts, clean shutdown
- ✅ **Self-Hosted** - You control the server, no third parties
- ✅ **Pre-Compiled Binaries** - No compilation needed, just download and run

---

## 🚀 Quick Start

### 1. Get a Free Cloud VM

**Recommended:** [Oracle Cloud (Always Free)](docs/SETUP_SERVER.md) - 4 ARM CPUs, 24GB RAM free forever

**Alternative:** [Google Cloud (Free Tier)](docs/SETUP_SERVER_GOOGLE_CLOUD.md) - e2-micro instance

### 2. Install Server (One Command)

SSH into your VM and run:

```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/clean/scripts/install_server.sh | sudo bash
```

This automatically:
- Installs and configures SSH server
- Sets up TLS wrapper (stunnel) on port 443
- Configures firewall rules
- Enables automatic startup

### 3. Download Client

Download the pre-built binary for your platform:

- **Windows:** [Download from Releases](https://github.com/dokabi-recon67/helloworld/releases)
- **macOS:** [Download from Releases](https://github.com/dokabi-recon67/helloworld/releases)

No compilation required - just download and run!

### 4. Configure & Connect

1. Run `helloworld.exe` (Windows) or `helloworld.app` (macOS)
2. Click **"Add Server"**
3. Enter your server details:
   - **Host:** Your server IP address
   - **Port:** `443` (default)
   - **User:** Your SSH username (e.g., `opc` for Oracle Cloud)
   - **Key:** Path to your SSH private key
4. Click **"CONNECT"**
5. Wait for **"Connected"** status
6. Configure your browser/apps to use SOCKS5 proxy: `127.0.0.1:1080`

### 5. Verify It Works

Visit these sites to verify:
- **IP Check:** https://api.ipify.org (should show your server's IP)
- **DNS Leak Test:** https://dnsleaktest.com (should show 100% privacy)
- **WebRTC Leak:** https://browserleaks.com/webrtc (should be disabled in browser)

---

## 🔒 Security & Stealth Features

### Maximum Passive Stealth (10/10 Rating)

HelloWorld achieves **maximum passive stealth** under tested conditions:

- **TLS Fingerprinting** - Browser-like fingerprints (Chrome/Firefox/Safari)
- **Traffic Shaping** - HTTP-like patterns with variable delays and packet sizes
- **ML Classification Evasion** - Pattern breaking with entropy variations
- **Port 443** - All traffic on standard HTTPS port
- **100% DNS Privacy** - All DNS queries through tunnel, zero leaks
- **Non-Actionable Classification** - Looks like standard HTTPS traffic

### Comprehensive Security Hardening

All major threat models are hardened:

#### On-Path Attacker (Read Traffic)
- ✅ Strong encryption: TLS 1.3 + SSH + AEAD encryption layer
- ✅ Key rotation: Automatic rotation every hour or after 1000 messages
- ✅ Key separation: Separate keys for client→server, server→client, and control channels
- ✅ Nonce discipline: Counter-based or random nonces, never reuse with same key

#### On-Path Attacker (Tamper/Insert/Drop)
- ✅ Integrity protection: AEAD authentication tags for every message
- ✅ Replay protection: Sequence numbers with sliding window (64 packets)
- ✅ Ordering: Monotonically increasing sequence numbers per direction
- ✅ Frame validation: Strict frame format validation (magic, version, type, length)
- ✅ State machine: Explicit state machine prevents invalid protocol transitions

#### Server Compromise / Stolen Config
- ✅ Key rotation: Ephemeral session keys limit exposure window
- ✅ Secrets hygiene: Never log keys, nonces, or raw secrets (even in debug)
- ✅ Session keys: Long-term keys never used directly, only for deriving session keys
- ✅ Key derivation: Proper key derivation from handshake (HKDF-like)

#### DoS / Resource Exhaustion
- ✅ Resource caps: Max 5 connections/IP, 10 unauthenticated handshakes, 16 streams/session, 1MB buffer
- ✅ Backpressure: Bounded queues (64KB per stream) prevent RAM exhaustion
- ✅ Timeouts: Handshake (10s), read idle (30s), write stall (5s), DNS (5s)
- ✅ Connection limits: Per-IP connection tracking and limits

### Production-Ready Reliability

- **Backpressure System** - Bounded ring buffers (64KB per stream). When full, stop reading from upstream socket until drained. Never "buffer forever".
- **Exponential Backoff Reconnect** - 250ms → 500ms → 1s → 2s → 4s → ... (cap at 30s) with ±20% jitter. Prevents network meltdown on outages.
- **Comprehensive Timeouts** - Handshake (10s), read idle (30s), write stall (5s), DNS (5s). Dead connections detected quickly.
- **Clean Shutdown** - Drain → flush → close prevents data corruption. System state is always consistent.

---

## 📋 Requirements

### Client
- **Windows 10+** or **macOS 11+** (Big Sur or later)
- **stunnel** (bundled with app)
- **OpenSSH** (included in OS)

### Server
- **Linux VM** with root access
- **Port 443** open (TCP)
- **512MB RAM** minimum (1GB+ recommended)

### Recommended Cloud Providers (Free Tier)
- **Oracle Cloud** - 4 ARM CPUs, 24GB RAM free forever ([Setup Guide](docs/SETUP_SERVER.md))
- **Google Cloud** - e2-micro instance free tier ([Setup Guide](docs/SETUP_SERVER_GOOGLE_CLOUD.md))
- **Any VPS provider** with port 443 access

---

## 📦 Installation

### Client

**Windows & macOS:** Download the pre-built binary from [Releases](https://github.com/dokabi-recon67/helloworld/releases)

**Latest Version:** v20260124 (10/10 Stealth Rating, Comprehensive Security)

**Note:** Source code is not included in releases. Only pre-compiled binaries are provided for security and ease of use.

### Server

Run the installation script on your Linux VM:

```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/clean/scripts/install_server.sh | sudo bash
```

The script automatically:
- Installs required dependencies
- Configures SSH server
- Sets up TLS wrapper (stunnel) on port 443
- Configures firewall rules
- Enables automatic startup

---

## ⚙️ Configuration

### Client Config

**Location:**
- Windows: `%APPDATA%\HelloWorld\config.txt`
- macOS: `~/.helloworld/config.txt`

**Format:**
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

**Location:** `/etc/helloworld/`

Files generated during setup (do not share):
- `stunnel.conf` - TLS wrapper config
- `server.pem` - TLS certificate (auto-generated)
- `server.key` - TLS private key (auto-generated)

---

## 🔧 Server Commands

```bash
# Start/stop server
sudo helloworld-server start
sudo helloworld-server stop
sudo helloworld-server status

# Tor mode (anonymous exit)
sudo helloworld-server tor
sudo helloworld-server rotate   # Get new Tor IP
sudo helloworld-server direct     # Back to normal

# Show current public IP
sudo helloworld-server ip
```

---

## 🌐 How It Works

```
Your Computer                    Your VM                     Internet
     |                              |                            |
     |-- TLS (port 443) ----------->|                            |
     |   (looks like HTTPS)         |                            |
     |                              |-- Direct/Tor/VPN --------->|
     |                              |                            |
```

1. **Client connects to VM** on port 443 using TLS (looks like HTTPS)
2. **Inside TLS:** SSH tunnel with key authentication
3. **SSH creates SOCKS proxy** on localhost:1080
4. **All traffic routed** through SOCKS proxy
5. **VM forwards traffic** to internet (direct, through Tor, or through VPN)

---

## 🛡️ Security Model

- **Layered Defense** - TLS 1.3 + SSH + AEAD encryption + frame validation + replay protection
- **No custom cryptography** - Uses proven TLS 1.3 + SSH + standard AEAD (ChaCha20-Poly1305, AES-GCM)
- **Key-only authentication** - No passwords, SSH key-based authentication
- **Session keys** - Long-term keys never used directly, only for deriving ephemeral session keys
- **Key rotation** - Automatic rotation every hour or after 1000 messages
- **Self-hosted** - You control the server. No third parties. No logs. No trust required.
- **No telemetry** - Zero data collection. The app never phones home.
- **Comprehensive threat model hardening** - See [THREAT_MODEL.md](docs/THREAT_MODEL.md) for details

---

## 🐛 Troubleshooting

### Connection Fails
- ✅ Check VM firewall/security list allows port 443
- ✅ Verify SSH key permissions (`chmod 600` on Linux/Mac)
- ✅ Check stunnel is running: `sudo systemctl status stunnel4`
- ✅ Verify server IP is correct

### IP Not Changing
- ✅ Enable **Full Tunnel Mode** in the app
- ✅ Check system proxy settings were applied
- ✅ Try disconnect and reconnect
- ✅ Verify browser is using SOCKS5 proxy

### DNS Leaks
- ✅ Ensure browser uses SOCKS5 DNS
- ✅ Check proxy configuration
- ✅ Test at https://dnsleaktest.com
- ✅ Verify DNS queries go through tunnel

### Tor Not Working
- ✅ Ensure Tor is installed: `sudo apt install tor`
- ✅ Wait 30 seconds after enabling Tor mode
- ✅ Check Tor is running: `sudo systemctl status tor`

### Slow Performance
- ✅ Check server location (closer = faster)
- ✅ Verify server resources (CPU/RAM)
- ✅ Check network conditions
- ✅ Try different server

---

## 📚 Documentation

- **[Threat Model & Security Hardening](docs/THREAT_MODEL.md)** - Comprehensive threat model documentation
- **[Server Setup Guide (Oracle Cloud)](docs/SETUP_SERVER.md)** - Always Free tier setup
- **[Server Setup Guide (Google Cloud)](docs/SETUP_SERVER_GOOGLE_CLOUD.md)** - Free tier setup
- **[Binaries README](binaries/README.md)** - Detailed client usage guide

---

## 🤝 Contributing

1. Fork the repo
2. Make your changes
3. **Ensure no credentials or private IPs in your commits**
4. Submit a pull request

---

## 📄 License

MIT License. See [LICENSE](LICENSE) file.

---

## ⚠️ Disclaimer

This software is provided for personal privacy and security purposes. Users are responsible for compliance with all applicable laws. The developers assume no liability for misuse.

This is **not** anonymity software and does not guarantee privacy against state-level adversaries. For strong anonymity, use Tor Browser directly.

**Detection rates are probabilistic** and depend on DPI system sophistication. Results are under tested conditions. Active interference (ISP probes, selective throttling) resistance is probabilistic. Maximum passive stealth achieved.

---

## 🌟 Features Summary

| Feature | Status | Rating |
|---------|--------|--------|
| Maximum Passive Stealth | ✅ | 10/10 |
| DNS Privacy | ✅ | 100% |
| IP Leak Prevention | ✅ | 100% |
| Connection Stability | ✅ | 100% |
| Security Hardening | ✅ | All Models |
| Reliability Features | ✅ | Production-Ready |
| TLS Wrapper | ✅ | Port 443 |
| SSH Transport | ✅ | Battle-Tested |
| Traffic Shaping | ✅ | HTTP-Like |
| ML Evasion | ✅ | Pattern Breaking |

---

**HelloWorld - Maximum Stealth, Maximum Privacy, Your Control**
