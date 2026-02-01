# HelloWorld

**DPI-resistant encrypted tunnel that looks like regular HTTPS traffic. Self-hosted with optional Tor integration for complete anonymity.**

[![Security](https://img.shields.io/badge/Security-10%2F10-brightgreen)](docs/THREAT_MODEL.md)
[![Stealth](https://img.shields.io/badge/Stealth-10%2F10-brightgreen)](docs/THREAT_MODEL.md)
[![License](https://img.shields.io/badge/License-MIT-blue)](LICENSE)

---

## 🎯 What Is This?

HelloWorld creates a **stealth tunnel** between your computer and a cloud VM you control. All traffic is wrapped in TLS on port 443, making it **indistinguishable from normal HTTPS browsing**.

**Not a VPN service.** You host everything yourself. No third parties. No logs. No trust required.

### Two Versions

| Version | Exit IP | Best For |
|---------|---------|----------|
| **v1 (Standard)** | Your server IP | Speed, general privacy |
| **v2 (Tor)** | Random Tor exit node | Maximum anonymity, IP rotation |

---

## 🚀 Quick Start

### 1. Get a Free Cloud VM

- **Oracle Cloud** - 4 ARM CPUs, 24GB RAM free forever
- **Google Cloud** - e2-micro free tier ($300 credit)

### 2. Install Server

**v1 (Standard) - Fast, stable IP:**
```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/clean/scripts/install_server.sh | sudo bash
```

**v2 (Tor) - Anonymous, rotating IP:**
```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/v2tor/scripts/install_server_tor.sh | sudo bash
```

### 3. Add SSH Key
```bash
echo 'YOUR_PUBLIC_KEY' >> ~/.ssh/authorized_keys
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys
```

### 4. Check Status
```bash
helloworld-status
```

### 5. Download Client & Connect

1. Download `helloworld.exe` from [Releases](https://github.com/dokabi-recon67/helloworld/releases) or `clean/binaries/windows/`
2. Run and click **Add Server**
3. Enter your server IP, port 443, SSH key path
4. Click **CONNECT**
5. Configure apps to use SOCKS5 proxy: `127.0.0.1:1080`

---

## 📊 Live Test Results (January 2026)

### v1 (Standard)
| Metric | Result |
|--------|--------|
| IP Leak Prevention | 100% ✅ |
| DNS Privacy | 100% ✅ |
| Reliability | 100% (50/50 requests) ✅ |
| Avg Latency | 1,276 ms |
| Exit IP | Your server IP |

### v2 (Tor Edition)
| Metric | Result |
|--------|--------|
| IP Leak Prevention | 100% ✅ |
| DNS Privacy | 100% ✅ |
| Reliability | 100% (50/50 requests) ✅ |
| Avg Latency | 2,111 ms |
| Exit IP | Random Tor node (NL, DE, SE, HK observed) |
| IP Rotation | Every 100 requests ✅ |

---

## 🔄 v1 vs v2 Comparison

| Feature | v1 (Standard) | v2 (Tor) |
|---------|---------------|----------|
| **Setup** | Simple | + Tor + redsocks |
| **Speed** | Fast (~1.3s) | Moderate (~2.1s) |
| **Exit IP** | Your server | Tor network |
| **IP Rotation** | None | Every 100 requests |
| **Anonymity** | Server-level | Network-level |
| **Traceability** | Server traceable | Untraceable |
| **Use Case** | General privacy | Maximum anonymity |

### When to Use Which?

**Use v1 if you need:**
- Maximum speed
- Stable IP for sessions
- Gaming/streaming
- General privacy

**Use v2 if you need:**
- Complete anonymity
- IP rotation
- Bypass IP bans
- Sensitive research

---

## 🏗️ Architecture

### v1 (Standard)
```
Your PC → TLS (443) → Your Server → Internet
                         ↑
                   Exit: Server IP
```

### v2 (Tor)
```
Your PC → TLS (443) → Your Server → Tor Network → Internet
                                        ↑
                                  Exit: Random Tor Node
                                  (Rotates every 100 requests)
```

---

## 🔒 Security Features

### Encryption
- **TLS 1.3** - Outer wrapper on port 443
- **SSH** - Authenticated tunnel with Ed25519 keys
- **Tor** (v2 only) - Additional 3 layers of onion routing

### Stealth
- Traffic looks like normal HTTPS
- Browser-like TLS fingerprints (Chrome, Firefox, Safari)
- Undetectable by DPI

### Privacy
- Self-hosted (you control everything)
- No logs possible
- Zero telemetry
- DNS queries through tunnel

---

## 📁 Repository Structure

```
helloworld/
├── clean/                      ← v1 STABLE
│   ├── binaries/windows/       ← Windows client
│   ├── scripts/                ← Server install script
│   └── README.md               ← v1 documentation
│
├── v2tor/                      ← v2 TOR EDITION
│   ├── binaries/windows/       ← Windows client (same)
│   ├── scripts/                ← Server install with Tor
│   ├── README.md               ← v2 documentation
│   └── SECURITY_REPORT_V2_TOR.md ← Detailed security report
│
├── client/                     ← Client source code
├── server/                     ← Server source code
└── docs/                       ← Documentation
```

---

## 🔧 Server Commands

### v1 (Standard)
```bash
helloworld-status              # Check status
sudo systemctl restart helloworld-stunnel  # Restart
```

### v2 (Tor)
```bash
helloworld-status              # Check status + IPs
helloworld-newip               # Force new Tor exit IP
sudo systemctl restart tor     # Restart Tor
```

---

## 💻 Requirements

### Client
- Windows 10+
- **helloworld.exe**: ~60 KB (written in C)
- ~60 MB RAM

### Server
- Linux VM (Debian/Ubuntu)
- Port 443 open
- **install_server.sh**: ~4 KB (very lightweight)
- 1 GB RAM (v2 with Tor)

### Free Cloud Options
- **Oracle Cloud** - Always Free tier
- **Google Cloud** - $300 free credit
- **AWS** - 12-month free tier

---

## 📚 Documentation

- **[v1 Security Report](clean/HELLOWORLD_SECURITY_REPORT_JANUARY_2026.md)** - Standard version test results
- **[v2 Tor Security Report](v2tor/SECURITY_REPORT_V2_TOR.md)** - Tor edition test results
- **[Investor Report](INVESTOR_REPORT_HELLOWORLD_V2.md)** - Comprehensive technical overview
- **[Threat Model](docs/THREAT_MODEL.md)** - Security architecture

---

## 🛡️ Security Model

| Threat | Protection |
|--------|------------|
| ISP monitoring | ✅ TLS on port 443 |
| Traffic analysis | ✅ Browser-like fingerprints |
| IP tracking | ✅ Server IP (v1) / Tor exit (v2) |
| DNS leaks | ✅ All DNS through tunnel |
| MITM attacks | ✅ TLS + SSH encryption |
| Server compromise | ✅ Ephemeral session keys |

---

## 🐛 Troubleshooting

### Connection Fails
- Check server firewall allows port 443
- Verify SSH key is in `~/.ssh/authorized_keys`
- Run `helloworld-status` on server

### IP Not Changing (v2)
- Check redsocks: `sudo systemctl status redsocks`
- Re-run routing: `sudo /etc/helloworld/setup-tor-routing.sh`

### Slow Connection (v2)
- Normal for Tor (~2s latency)
- Try `helloworld-newip` for different exit

---

## 📄 License

MIT License. See [LICENSE](LICENSE) file.

---

## ⚠️ Disclaimer

This software is for personal privacy and security. Users responsible for legal compliance. Not anonymity software against state-level adversaries without v2 Tor mode.

---

## 🌟 Features Summary

| Feature | v1 | v2 |
|---------|----|----|
| Stealth (Port 443) | ✅ 10/10 | ✅ 10/10 |
| DNS Privacy | ✅ 100% | ✅ 100% |
| IP Leak Prevention | ✅ 100% | ✅ 100% |
| Connection Reliability | ✅ 100% | ✅ 100% |
| Exit IP Anonymity | ⚠️ Server | ✅ Tor |
| IP Rotation | ❌ | ✅ Every 100 req |
| Speed | ⚡ Fast | 🐢 Moderate |

---

**HelloWorld - Maximum Stealth, Maximum Privacy, Your Control**

**v1** for speed | **v2** for anonymity
