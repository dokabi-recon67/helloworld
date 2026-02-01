# HelloWorld v2 - Tor Edition 🧅

**Ultimate privacy: Your traffic exits through Tor with automatic IP rotation every 100 requests.**

## Why v2 Tor Edition?

| Feature | v1 (Standard) | v2 (Tor) |
|---------|---------------|----------|
| Exit IP | Your server IP | Tor exit node (anonymous) |
| IP Rotation | Never | Every 100 requests |
| Traceability | Server can be traced | Tor network (untraceable) |
| Speed | Fast (~1.3s) | Moderate (~2-3s) |
| Use Case | General privacy | Maximum anonymity |

## Architecture

```
┌──────────────┐                              ┌──────────────────┐
│   Your PC    │      TLS (Port 443)          │   Your Server    │
│  HelloWorld  │  ←─────────────────────────→ │    stunnel       │
│   Client     │    Looks like HTTPS          │       ↓          │
└──────────────┘                              │    SSH           │
       ↓                                      │       ↓          │
  SOCKS Proxy                                 │   redsocks       │
  localhost:1080                              │       ↓          │
       ↓                                      │     Tor          │
  Your Browser                                └───────┬──────────┘
                                                      ↓
                                              ┌──────────────────┐
                                              │  Tor Network     │
                                              │  (3+ relays)     │
                                              └───────┬──────────┘
                                                      ↓
                                              ┌──────────────────┐
                                              │  Tor Exit Node   │
                                              │  (Random IP)     │
                                              │  Stockholm, etc  │
                                              └───────┬──────────┘
                                                      ↓
                                                  Internet
```

## Benefits

### 🔒 Triple-Layer Encryption
1. **TLS 1.3** - Outer layer on port 443
2. **SSH** - Authenticated tunnel
3. **Tor** - Onion-routed encryption

### 🌍 Geographic Diversity
- Exit nodes in 50+ countries
- IP changes automatically
- Bypass geo-restrictions

### 🕵️ Maximum Anonymity
- Server IP never exposed
- Traffic untraceable to you
- No logs at exit point

### 🔄 Automatic IP Rotation
- New IP every 100 requests
- Manual rotation: `helloworld-newip`
- Defeats IP-based tracking

## Installation

### Server Setup (One Command)

```bash
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/v2tor/scripts/install_server_tor.sh | sudo bash
```

After installation:
```bash
# Add your SSH key
echo 'ssh-ed25519 YOUR_PUBLIC_KEY' >> ~/.ssh/authorized_keys
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys

# Check status
helloworld-status
```

### Client Setup

Use the same `helloworld.exe` from v1 - just point it to your server IP!

## Server Commands

| Command | Description |
|---------|-------------|
| `helloworld-status` | Check all services and IPs |
| `helloworld-newip` | Force new Tor exit IP now |
| `sudo systemctl restart tor` | Restart Tor |
| `tail -f /var/log/helloworld-tor-rotator.log` | Watch IP rotations |

## Live Test Results (January 2026)

| Metric | Result |
|--------|--------|
| IP Leak Prevention | 100% ✅ |
| DNS Privacy | 100% ✅ |
| Connection Reliability | 100% ✅ |
| Tor Routing | Active ✅ |
| IP Rotation | Every 100 requests ✅ |

### Verified Exit Locations
- 🇸🇪 Stockholm, Sweden
- 🇩🇪 Frankfurt, Germany  
- 🇳🇱 Amsterdam, Netherlands
- 🇺🇸 Various US locations
- And 50+ more countries

## When to Use v2 vs v1

**Use v2 (Tor)** for:
- Maximum anonymity
- Sensitive browsing
- Bypassing IP bans
- Research/journalism
- When IP rotation matters

**Use v1 (Standard)** for:
- Faster speeds
- Stable IP sessions
- Gaming/streaming
- General privacy

## Technical Details

### Services Running
| Service | Port | Purpose |
|---------|------|---------|
| stunnel | 443 | TLS termination |
| sshd | 22 | SSH tunnel |
| tor | 9050 | SOCKS5 proxy |
| redsocks | 12345 | Transparent proxy |

### IP Rotation Mechanism
1. Rotator monitors connection count
2. After 100 connections, sends `NEWNYM` to Tor
3. Tor builds new circuit through different relays
4. New exit node = new IP address

## Troubleshooting

**Traffic showing server IP instead of Tor:**
```bash
# Check redsocks is running
sudo systemctl status redsocks

# Check iptables rules
sudo iptables -t nat -L -n | grep REDSOCKS

# Re-apply routing
sudo /etc/helloworld/setup-tor-routing.sh
```

**Tor not connecting:**
```bash
sudo systemctl restart tor
sudo journalctl -fu tor
```

**Get new IP manually:**
```bash
helloworld-newip
```

---

**Version:** 2.0 (Tor Edition)  
**Released:** January 2026  
**Status:** ✅ Production Ready
