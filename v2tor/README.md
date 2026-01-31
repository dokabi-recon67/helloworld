# HelloWorld v2 - Tor Edition

**Automatic IP rotation every 100 requests through Tor network.**

## What's Different from v1?

| Feature | v1 (clean) | v2 (Tor) |
|---------|------------|----------|
| Exit IP | Your server IP | Tor exit node IP |
| IP Rotation | None (static) | Every 100 requests |
| Anonymity | Server-level | Network-level (Tor) |
| Speed | Fast | Moderate (Tor overhead) |

## Architecture

```
┌─────────────────┐                           ┌─────────────────┐
│  Your Device    │     TLS (Port 443)        │   Your Server   │
│  HelloWorld     │ ←------------------------→│   stunnel + SSH │
│  Client         │   Looks like HTTPS        │        ↓        │
└─────────────────┘                           │      Tor        │
                                              │        ↓        │
                                              │  Tor Network    │
                                              └────────┬────────┘
                                                       ↓
                                              ┌─────────────────┐
                                              │  Tor Exit Node  │
                                              │  (Rotating IP)  │
                                              └─────────────────┘
                                                       ↓
                                                   Internet
```

## IP Rotation

- **Automatic rotation** after every 100 requests
- **No action required** - rotation happens in background
- **New Tor circuit** = New exit IP address
- Logs available at `/var/log/helloworld-tor-rotator.log`

## Installation

### Server Setup

```bash
# One-line install
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/v2tor/scripts/install_server_tor.sh | sudo bash

# Add your SSH key
echo 'YOUR_PUBLIC_KEY' >> ~/.ssh/authorized_keys
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys

# Check status
helloworld-status
```

### Client Setup

Use the same `helloworld.exe` from v1 - no changes needed!

The client connects to your server the same way. The Tor routing happens on the server side.

## Commands (on server)

```bash
# Check overall status
helloworld-status

# View rotation log
tail -f /var/log/helloworld-tor-rotator.log

# Manual circuit rotation
echo -e 'AUTHENTICATE ""\r\nSIGNAL NEWNYM\r\nQUIT' | nc 127.0.0.1 9051

# Check current Tor exit IP
curl --socks5 127.0.0.1:9050 https://api.ipify.org
```

## Services

| Service | Port | Purpose |
|---------|------|---------|
| stunnel | 443 | TLS wrapper |
| sshd | 22 | SSH tunnel |
| tor | 9050 | SOCKS proxy |
| tor | 9051 | Control port |
| tor | 9040 | Transparent proxy |

## When to Use v2 vs v1

**Use v1 (clean)** if you need:
- Maximum speed
- Stable IP for sessions
- Simple setup

**Use v2 (Tor)** if you need:
- IP rotation/anonymity
- Bypass IP-based blocking
- Extra privacy layer

## Troubleshooting

**Tor not starting:**
```bash
sudo systemctl status tor
sudo journalctl -u tor
```

**Rotation not working:**
```bash
sudo systemctl restart helloworld-tor-rotate
sudo journalctl -u helloworld-tor-rotate
```

**Check Tor connectivity:**
```bash
curl --socks5 127.0.0.1:9050 https://api.ipify.org
```

---

**Version:** 2.0 (Tor Edition)  
**Released:** January 2026

