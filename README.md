# HelloWorld

DPI-resistant encrypted tunnel that looks like regular HTTPS traffic. Self-hosted on your own cloud VM.

## What Is This

HelloWorld creates a stealth tunnel between your computer and a cloud VM you control. All traffic is wrapped in TLS on port 443, making it indistinguishable from normal HTTPS browsing.

**Not a VPN service.** You host everything yourself. No third parties. No logs. No trust required.

## Features

- TLS wrapper on port 443 (looks like HTTPS)
- SSH transport (battle-tested encryption)
- Full tunnel mode (all traffic through VM)
- Proxy mode (SOCKS5 for specific apps)
- Tor exit mode with circuit rotation
- Kill switch (blocks traffic if tunnel drops)
- Cross-platform (Windows, macOS)
- Written in C (fast, no dependencies)
- Modern GUI with tutorial for beginners

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

## Security Notice

### What's Safe to Share
- This repository (no credentials stored)
- The HelloWorld app itself
- Your server's public IP (if you want others to connect)

### NEVER Share or Commit
- SSH private keys (`.key`, `.pem`, `id_rsa`, etc.)
- Your `config.json` or `config.txt` files
- Server TLS certificates with private keys
- Anything in your `%APPDATA%\HelloWorld\` or `~/.helloworld/` folder

The `.gitignore` in this repo blocks sensitive files, but **always double-check before pushing any forks**.

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

- **No custom cryptography** - Uses proven TLS 1.3 + SSH
- **Key-only authentication** - No passwords
- **Self-hosted** - You control the server
- **Open source** - Fully auditable code
- **No telemetry** - Zero data collection

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

## Disclaimer

This software is provided for personal privacy and security purposes. Users are responsible for compliance with all applicable laws. The developers assume no liability for misuse.

This is **not** anonymity software and does not guarantee privacy against state-level adversaries. For strong anonymity, use Tor Browser directly.
