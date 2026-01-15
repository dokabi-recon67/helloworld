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
- Cross-platform (Windows, macOS)
- Written in C (fast, no dependencies)

## Requirements

### Client
- Windows 10+ or macOS 11+
- stunnel (bundled)
- OpenSSH (included in OS)

### Server
- Linux VM with root access
- Port 443 open
- 512MB RAM minimum

### Recommended Cloud Providers
- Oracle Cloud Free Tier (best)
- Google Cloud Free Tier
- Any VPS provider

## Quick Start

### Server Setup

```bash
# Download and extract
wget https://github.com/yourusername/helloworld/releases/latest/download/helloworld-server.tar.gz
tar xzf helloworld-server.tar.gz
cd helloworld-server

# Build
mkdir build && cd build
cmake ..
make
sudo make install

# Setup and start
sudo helloworld-server setup
sudo helloworld-server start
```

### Add Your SSH Key

```bash
# On your local machine, generate a key if needed
ssh-keygen -t ed25519 -f ~/.ssh/helloworld

# Copy public key to server
cat ~/.ssh/helloworld.pub | ssh root@your-vm "cat >> ~/.ssh/authorized_keys"
```

### Client Setup

1. Download the client for your OS from Releases
2. Extract and run HelloWorld
3. Add your server:
   - Name: Any name you want
   - Host: Your VM's IP or hostname
   - Port: 443
   - Key: Path to your SSH private key
   - User: root (or your VM username)
4. Click Connect

### Verify

Visit https://api.ipify.org - you should see your VM's IP address.

## Building From Source

### Client (Windows)

```powershell
cd client
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Client (macOS)

```bash
cd client
mkdir build && cd build
cmake ..
make
```

### Server (Linux)

```bash
cd server
mkdir build && cd build
cmake ..
make
sudo make install
```

## Server Commands

```bash
# Start server
sudo helloworld-server start

# Stop server
sudo helloworld-server stop

# Check status
sudo helloworld-server status

# Switch to Tor exit mode
sudo helloworld-server tor

# Rotate Tor circuit (get new IP)
sudo helloworld-server rotate

# Switch back to direct mode
sudo helloworld-server direct

# Show current public IP
sudo helloworld-server ip
```

## Configuration

### Client Config

Location:
- Windows: `%APPDATA%\HelloWorld\config.txt`
- macOS: `~/.helloworld/config.txt`

Format:
```ini
mode = proxy
killswitch = 0
selected = 0

[server]
name = My Server
host = 192.168.1.100
port = 443
key = C:\Users\me\.ssh\helloworld
user = root
```

### Server Config

Location: `/etc/helloworld/`

Files:
- `stunnel.conf` - TLS wrapper config
- `server.pem` - TLS certificate
- `server.key` - TLS private key

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

## Security

- No custom cryptography
- TLS 1.3 via stunnel
- SSH with key authentication only
- No passwords
- All code is open source and auditable

## Troubleshooting

### Connection fails
- Check VM firewall allows port 443
- Verify SSH key permissions (600)
- Check stunnel is running on server

### IP not changing
- Enable "Full Tunnel Mode" in client
- Verify system proxy settings applied
- Try disconnecting and reconnecting

### Tor not working
- Ensure Tor is installed on server
- Check Tor control port (9051) is accessible locally
- Wait 30 seconds after enabling Tor mode

## License

MIT License. See LICENSE file.

## Disclaimer

This software is for personal use only. Users are responsible for compliance with local laws. This is not anonymity software and should not be used for illegal activities.

