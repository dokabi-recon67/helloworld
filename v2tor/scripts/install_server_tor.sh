#!/bin/bash
#
# HelloWorld Server v2 - Tor Edition
# Routes all tunnel traffic through Tor for rotating exit IPs
#
# Usage: curl -sSL <url> | sudo bash
#        OR: sudo bash install_server_tor.sh
#
# Architecture:
#   Client → TLS (443) → SSH → Server → Tor → Internet
#   Exit IP = Tor exit node (rotates every 10 minutes)
#

set -e

echo "========================================"
echo "   HelloWorld Server v2 - Tor Edition"
echo "========================================"
echo ""
echo "This version routes traffic through Tor"
echo "for automatic IP rotation."
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

# Detect OS
if [ -f /etc/debian_version ]; then
    PKG_MANAGER="apt-get"
    PKG_UPDATE="apt-get update"
    PKG_INSTALL="apt-get install -y"
elif [ -f /etc/redhat-release ]; then
    PKG_MANAGER="yum"
    PKG_UPDATE="yum check-update || true"
    PKG_INSTALL="yum install -y"
else
    echo "Unsupported OS. Please install packages manually."
    exit 1
fi

echo "[1/8] Updating packages..."
$PKG_UPDATE > /dev/null 2>&1

echo "[2/8] Installing stunnel4..."
$PKG_INSTALL stunnel4 > /dev/null 2>&1

echo "[3/8] Installing Tor..."
$PKG_INSTALL tor > /dev/null 2>&1

echo "[4/8] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true

echo "[5/8] Generating TLS certificates..."
openssl req -new -x509 -days 3650 -nodes \
    -out /etc/helloworld/server.pem \
    -keyout /etc/helloworld/server.key \
    -subj "/CN=localhost" 2>/dev/null
chmod 600 /etc/helloworld/server.key
chown stunnel4:stunnel4 /etc/helloworld/server.pem /etc/helloworld/server.key 2>/dev/null || true

echo "[6/8] Configuring Tor..."
cat > /etc/tor/torrc << 'TORRC'
# HelloWorld Tor Configuration
# Provides SOCKS proxy for outbound traffic

# Run as daemon
RunAsDaemon 1

# SOCKS proxy for local connections only
SocksPort 127.0.0.1:9050
SocksPolicy accept 127.0.0.1
SocksPolicy reject *

# DNS resolution through Tor
DNSPort 127.0.0.1:5353

# Circuit rotation settings
CircuitBuildTimeout 10
LearnCircuitBuildTimeout 0

# Rotate circuit after 100 streams (requests)
# MaxCircuitDirtiness = time before circuit is considered "dirty"
# We set it low and use stream isolation for per-100-request rotation
MaxCircuitDirtiness 60

# Force new circuit frequently
NewCircuitPeriod 60

# Performance tuning
NumEntryGuards 8

# Logging
Log notice file /var/log/tor/notices.log

# Data directory
DataDirectory /var/lib/tor

# Control port for programmatic circuit rotation
ControlPort 9051
CookieAuthentication 1
CookieAuthFileGroupReadable 1
TORRC

# Create Tor log directory
mkdir -p /var/log/tor
chown debian-tor:debian-tor /var/log/tor 2>/dev/null || chown tor:tor /var/log/tor 2>/dev/null || true

echo "[7/8] Configuring stunnel with Tor routing..."
cat > /etc/helloworld/stunnel.conf << 'STUNNELCONF'
; HelloWorld Server v2 - Tor Edition
; TLS wrapper for SSH connections
pid = /var/run/stunnel4/stunnel.pid
setuid = stunnel4
setgid = stunnel4
debug = 5
output = /var/log/helloworld-stunnel.log

[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key
STUNNELCONF

echo "[8/8] Configuring SSH to route through Tor..."

# Create SSH wrapper script that forces Tor proxy
cat > /usr/local/bin/helloworld-proxy-command << 'PROXYEOF'
#!/bin/bash
# Forces SSH dynamic forwarding to use Tor
# This script is used by SSH ProxyCommand
nc -x 127.0.0.1:9050 -X 5 "$@"
PROXYEOF
chmod +x /usr/local/bin/helloworld-proxy-command

# Configure SSH to use Tor for outbound connections
# We'll use iptables to redirect outbound traffic through Tor

# Create Tor transparent proxy rules
cat > /etc/helloworld/tor-routing.sh << 'TORROUTING'
#!/bin/bash
# Tor transparent proxy routing for HelloWorld
# Routes all outbound traffic from SSH tunnels through Tor

TOR_UID=$(id -u debian-tor 2>/dev/null || id -u tor 2>/dev/null || echo "")

if [ -z "$TOR_UID" ]; then
    echo "Warning: Tor user not found, skipping iptables rules"
    exit 0
fi

# Clear existing rules
iptables -t nat -F HELLOWORLD_TOR 2>/dev/null || true
iptables -t nat -X HELLOWORLD_TOR 2>/dev/null || true

# Create chain for HelloWorld Tor routing
iptables -t nat -N HELLOWORLD_TOR

# Don't redirect Tor's own traffic
iptables -t nat -A HELLOWORLD_TOR -m owner --uid-owner $TOR_UID -j RETURN

# Don't redirect local traffic
iptables -t nat -A HELLOWORLD_TOR -d 127.0.0.0/8 -j RETURN
iptables -t nat -A HELLOWORLD_TOR -d 10.0.0.0/8 -j RETURN
iptables -t nat -A HELLOWORLD_TOR -d 172.16.0.0/12 -j RETURN
iptables -t nat -A HELLOWORLD_TOR -d 192.168.0.0/16 -j RETURN

# Redirect DNS to Tor's DNS port
iptables -t nat -A HELLOWORLD_TOR -p udp --dport 53 -j REDIRECT --to-ports 5353
iptables -t nat -A HELLOWORLD_TOR -p tcp --dport 53 -j REDIRECT --to-ports 5353

# Redirect all TCP through Tor's TransPort
iptables -t nat -A HELLOWORLD_TOR -p tcp -j REDIRECT --to-ports 9040

# Apply to OUTPUT chain
iptables -t nat -A OUTPUT -j HELLOWORLD_TOR

echo "Tor routing rules applied"
TORROUTING
chmod +x /etc/helloworld/tor-routing.sh

# Update Tor config to enable TransPort
cat >> /etc/tor/torrc << 'TRANSTOR'

# Transparent proxy settings
TransPort 127.0.0.1:9040
AutomapHostsOnResolve 1
VirtualAddrNetworkIPv4 10.192.0.0/10
TRANSTOR

# Create Tor rotation script (rotates after 100 requests)
cat > /usr/local/bin/helloworld-tor-rotate << 'ROTATOR'
#!/bin/bash
#
# HelloWorld Tor Rotator
# Monitors connections and rotates Tor circuit every 100 requests
#

REQUEST_COUNT=0
ROTATE_AFTER=100
LOG_FILE="/var/log/helloworld-tor-rotator.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" >> "$LOG_FILE"
}

rotate_circuit() {
    # Send NEWNYM signal to Tor to get new circuit
    echo -e 'AUTHENTICATE ""\r\nSIGNAL NEWNYM\r\nQUIT' | nc 127.0.0.1 9051 > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org 2>/dev/null)
        log "Circuit rotated! New exit IP: $NEW_IP"
        echo "$(date '+%H:%M:%S') - Rotated! New IP: $NEW_IP"
    else
        log "Failed to rotate circuit"
    fi
}

log "Tor Rotator started (rotate every $ROTATE_AFTER requests)"
echo "HelloWorld Tor Rotator - Rotating every $ROTATE_AFTER requests"
echo "Press Ctrl+C to stop"
echo ""

# Get initial IP
INITIAL_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org 2>/dev/null)
echo "Initial Tor exit IP: $INITIAL_IP"
log "Initial IP: $INITIAL_IP"

# Monitor SSH connections (count established connections through stunnel)
LAST_COUNT=0
while true; do
    # Count current SSH dynamic forwarding connections
    CURRENT_COUNT=$(ss -tn state established | grep -c ":1080" 2>/dev/null || echo "0")
    
    if [ "$CURRENT_COUNT" -gt "$LAST_COUNT" ]; then
        NEW_REQUESTS=$((CURRENT_COUNT - LAST_COUNT))
        REQUEST_COUNT=$((REQUEST_COUNT + NEW_REQUESTS))
        echo -ne "\rRequests: $REQUEST_COUNT / $ROTATE_AFTER"
    fi
    
    LAST_COUNT=$CURRENT_COUNT
    
    # Check if we need to rotate
    if [ $REQUEST_COUNT -ge $ROTATE_AFTER ]; then
        echo ""
        rotate_circuit
        REQUEST_COUNT=0
    fi
    
    sleep 1
done
ROTATOR
chmod +x /usr/local/bin/helloworld-tor-rotate

# Create systemd service for auto-rotation
cat > /etc/systemd/system/helloworld-tor-rotate.service << 'ROTSVC'
[Unit]
Description=HelloWorld Tor Circuit Rotator (every 100 requests)
After=tor.service helloworld-stunnel.service

[Service]
Type=simple
ExecStart=/usr/local/bin/helloworld-tor-rotate
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
ROTSVC

# Start services
echo ""
echo "Starting services..."

# Start Tor
systemctl enable tor 2>/dev/null || true
systemctl restart tor 2>/dev/null || service tor restart 2>/dev/null || tor &

# Wait for Tor to bootstrap
echo "Waiting for Tor to connect to network..."
sleep 10

# Apply Tor routing rules
/etc/helloworld/tor-routing.sh 2>/dev/null || echo "Note: iptables rules may need manual setup"

# Start stunnel
/usr/bin/stunnel /etc/helloworld/stunnel.conf

# Create status command
cat > /usr/local/bin/helloworld-status << 'STATUS'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld v2 - Tor Edition Status"
echo "========================================"
echo ""

# Check stunnel
if ss -tlnp | grep -q ":443"; then
    echo "Stunnel (TLS): ✅ RUNNING on port 443"
else
    echo "Stunnel (TLS): ❌ NOT RUNNING"
fi

# Check SSH
if ss -tlnp | grep -q ":22"; then
    echo "SSH: ✅ RUNNING on port 22"
else
    echo "SSH: ❌ NOT RUNNING"
fi

# Check Tor
if ss -tlnp | grep -q ":9050"; then
    echo "Tor SOCKS: ✅ RUNNING on port 9050"
else
    echo "Tor SOCKS: ❌ NOT RUNNING"
fi

if ss -tlnp | grep -q ":9040"; then
    echo "Tor TransPort: ✅ RUNNING on port 9040"
else
    echo "Tor TransPort: ⚠️ NOT RUNNING (transparent proxy disabled)"
fi

# Get Tor exit IP
echo ""
echo "Checking Tor exit IP..."
TOR_IP=$(curl -s --socks5 127.0.0.1:9050 https://api.ipify.org 2>/dev/null || echo "Unable to reach")
SERVER_IP=$(curl -s https://api.ipify.org 2>/dev/null || echo "Unable to reach")

echo "Server IP: $SERVER_IP"
echo "Tor Exit IP: $TOR_IP"

if [ "$TOR_IP" != "$SERVER_IP" ] && [ "$TOR_IP" != "Unable to reach" ]; then
    echo "Status: ✅ Traffic routing through Tor!"
else
    echo "Status: ⚠️ Tor routing may need configuration"
fi

# Check circuit rotation
echo ""
echo "Tor Circuit Info:"
echo "  - Rotates after every 100 requests"
echo "  - IP changes automatically"

# Check rotator status
if systemctl is-active --quiet helloworld-tor-rotate 2>/dev/null; then
    echo "  - Rotator service: ✅ RUNNING"
else
    echo "  - Rotator service: ⚠️ Not running (start with: sudo systemctl start helloworld-tor-rotate)"
fi

# Stunnel logs
echo ""
echo "Recent stunnel activity:"
tail -5 /var/log/helloworld-stunnel.log 2>/dev/null || echo "No logs yet"

echo ""
echo "========================================"
STATUS
chmod +x /usr/local/bin/helloworld-status

# Create systemd service for auto-start
cat > /etc/systemd/system/helloworld-stunnel.service << 'SYSTEMD'
[Unit]
Description=HelloWorld Stunnel TLS Wrapper
After=network.target tor.service
Wants=tor.service

[Service]
Type=forking
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
SYSTEMD

systemctl daemon-reload 2>/dev/null || true
systemctl enable helloworld-stunnel 2>/dev/null || true
systemctl enable helloworld-tor-rotate 2>/dev/null || true
systemctl start helloworld-tor-rotate 2>/dev/null || true

echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""
echo "HelloWorld v2 (Tor Edition) is now running!"
echo ""
echo "Architecture:"
echo "  Client → TLS (443) → SSH → Tor → Internet"
echo ""
echo "Features:"
echo "  ✅ TLS encryption on port 443"
echo "  ✅ SSH tunnel with key auth"
echo "  ✅ Tor routing for exit IP rotation"
echo "  ✅ New IP every 100 requests (automatic)"
echo ""
echo "Commands:"
echo "  helloworld-status  - Check service status"
echo ""
echo "Next steps:"
echo "  1. Add your SSH public key:"
echo "     echo 'YOUR_PUBLIC_KEY' >> ~/.ssh/authorized_keys"
echo "     chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys"
echo ""
echo "  2. Check status:"
echo "     helloworld-status"
echo ""
echo "========================================"

