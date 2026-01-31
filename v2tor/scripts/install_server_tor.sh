#!/bin/bash
#
# HelloWorld Server v2 - Tor Edition (SAFE VERSION)
# Routes tunnel traffic through Tor for rotating exit IPs
# NO iptables - won't lock you out of SSH!
#
# Usage: curl -sSL <url> | sudo bash
#        OR: sudo bash install_server_tor.sh
#
# Architecture:
#   Client → TLS (443) → SSH (with ProxyCommand) → Tor → Internet
#   Exit IP = Tor exit node (rotates every 100 requests)
#

set -e

echo "========================================"
echo "   HelloWorld Server v2 - Tor Edition"
echo "         (Safe Version - No iptables)"
echo "========================================"
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
    echo "Unsupported OS."
    exit 1
fi

echo "[1/7] Updating packages..."
$PKG_UPDATE > /dev/null 2>&1

echo "[2/7] Installing stunnel4..."
$PKG_INSTALL stunnel4 > /dev/null 2>&1

echo "[3/7] Installing Tor and netcat..."
$PKG_INSTALL tor netcat-openbsd > /dev/null 2>&1 || $PKG_INSTALL tor nc > /dev/null 2>&1

echo "[4/7] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true

echo "[5/7] Generating TLS certificates..."
openssl req -new -x509 -days 3650 -nodes \
    -out /etc/helloworld/server.pem \
    -keyout /etc/helloworld/server.key \
    -subj "/CN=localhost" 2>/dev/null
chmod 600 /etc/helloworld/server.key
chown stunnel4:stunnel4 /etc/helloworld/server.pem /etc/helloworld/server.key 2>/dev/null || true

echo "[6/7] Configuring Tor..."
cat > /etc/tor/torrc << 'TORRC'
# HelloWorld Tor Configuration v2
# SOCKS proxy for outbound traffic

RunAsDaemon 1

# SOCKS proxy
SocksPort 127.0.0.1:9050

# Control port for circuit rotation
ControlPort 127.0.0.1:9051
CookieAuthentication 0
HashedControlPassword

# Circuit settings - rotate frequently
MaxCircuitDirtiness 60
NewCircuitPeriod 60

# Performance
CircuitBuildTimeout 10
NumEntryGuards 8

# Logging
Log notice file /var/log/tor/notices.log
DataDirectory /var/lib/tor
TORRC

mkdir -p /var/log/tor
chown debian-tor:debian-tor /var/log/tor 2>/dev/null || chown tor:tor /var/log/tor 2>/dev/null || true

echo "[7/7] Configuring stunnel..."
cat > /etc/helloworld/stunnel.conf << 'STUNNELCONF'
; HelloWorld Server v2 - Tor Edition
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

# Create Tor rotation script (every 100 requests)
cat > /usr/local/bin/helloworld-tor-rotate << 'ROTATOR'
#!/bin/bash
#
# HelloWorld Tor Rotator - Rotates IP every 100 requests
#

ROTATE_AFTER=100
REQUEST_COUNT=0
LAST_CONNECTIONS=0
LOG="/var/log/helloworld-tor-rotator.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" | tee -a "$LOG"
}

rotate_circuit() {
    # Request new Tor circuit
    (echo authenticate '""'; echo signal newnym; echo quit) | nc 127.0.0.1 9051 > /dev/null 2>&1
    sleep 2
    NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null || echo "checking...")
    log "🔄 ROTATED! New Tor exit IP: $NEW_IP"
}

log "========================================="
log "Tor Rotator Started (every $ROTATE_AFTER requests)"
log "========================================="

# Get initial IP
sleep 3
CURRENT_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null || echo "connecting...")
log "Initial Tor exit IP: $CURRENT_IP"

while true; do
    # Count SSH tunnel connections (port 1080 SOCKS)
    CURRENT_CONNECTIONS=$(ss -tn 2>/dev/null | grep -c ":1080" || echo "0")
    
    if [ "$CURRENT_CONNECTIONS" -gt "$LAST_CONNECTIONS" ]; then
        DIFF=$((CURRENT_CONNECTIONS - LAST_CONNECTIONS))
        REQUEST_COUNT=$((REQUEST_COUNT + DIFF))
    fi
    LAST_CONNECTIONS=$CURRENT_CONNECTIONS
    
    # Rotate after 100 requests
    if [ $REQUEST_COUNT -ge $ROTATE_AFTER ]; then
        rotate_circuit
        REQUEST_COUNT=0
    fi
    
    sleep 2
done
ROTATOR
chmod +x /usr/local/bin/helloworld-tor-rotate

# Create systemd services
cat > /etc/systemd/system/helloworld-stunnel.service << 'SYSTEMD'
[Unit]
Description=HelloWorld Stunnel
After=network.target

[Service]
Type=forking
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
Restart=on-failure

[Install]
WantedBy=multi-user.target
SYSTEMD

cat > /etc/systemd/system/helloworld-tor-rotate.service << 'ROTSVC'
[Unit]
Description=HelloWorld Tor Rotator
After=tor.service

[Service]
Type=simple
ExecStart=/usr/local/bin/helloworld-tor-rotate
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
ROTSVC

# Create status command
cat > /usr/local/bin/helloworld-status << 'STATUS'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld v2 - Tor Edition"
echo "========================================"
echo ""

# Stunnel
if ss -tlnp 2>/dev/null | grep -q ":443"; then
    echo "✅ Stunnel: RUNNING (port 443)"
else
    echo "❌ Stunnel: NOT RUNNING"
fi

# SSH
if ss -tlnp 2>/dev/null | grep -q ":22"; then
    echo "✅ SSH: RUNNING (port 22)"
else
    echo "❌ SSH: NOT RUNNING"
fi

# Tor
if ss -tlnp 2>/dev/null | grep -q ":9050"; then
    echo "✅ Tor SOCKS: RUNNING (port 9050)"
else
    echo "❌ Tor SOCKS: NOT RUNNING"
fi

# Rotator
if pgrep -f "helloworld-tor-rotate" > /dev/null; then
    echo "✅ Rotator: RUNNING"
else
    echo "⚠️  Rotator: NOT RUNNING"
fi

echo ""
echo "========================================"
echo "IP Addresses (for client config)"
echo "========================================"

# Get REAL server IP (GCP metadata or direct curl without Tor)
# Try GCP metadata first (most reliable)
SERVER_IP=$(curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip 2>/dev/null)

# Fallback to direct curl if not on GCP
if [ -z "$SERVER_IP" ] || [ "$SERVER_IP" == "" ]; then
    SERVER_IP=$(curl -s --max-time 5 --noproxy '*' https://api.ipify.org 2>/dev/null || echo "error")
fi

# Get Tor exit IP (through Tor SOCKS)
TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org 2>/dev/null || echo "error")

echo ""
echo "  🖥️  SERVER IP: $SERVER_IP"
echo "      (Use this in HelloWorld client config)"
echo ""
echo "  🧅 TOR EXIT:  $TOR_IP"
echo "      (Your traffic appears from this IP)"
echo ""

if [ "$TOR_IP" != "error" ] && [ "$TOR_IP" != "$SERVER_IP" ]; then
    echo "✅ Tor is working! IPs are different."
else
    echo "⚠️  Warning: Check Tor status"
fi

echo ""
echo "========================================"
echo "Rotation: Every 100 requests"
echo "========================================"
echo ""
echo "Commands:"
echo "  helloworld-status         - This status"
echo "  sudo journalctl -fu tor   - Tor logs"
echo "  tail -f /var/log/helloworld-tor-rotator.log - Rotation log"
echo ""
echo "========================================"
STATUS
chmod +x /usr/local/bin/helloworld-status

# Start services
echo ""
echo "Starting services..."

systemctl daemon-reload
systemctl enable tor
systemctl restart tor

# Wait for Tor to bootstrap
echo "Waiting for Tor to connect (30 seconds)..."
sleep 30

# Check if Tor is working
TOR_TEST=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null)
if [ -n "$TOR_TEST" ]; then
    echo "✅ Tor connected! Exit IP: $TOR_TEST"
else
    echo "⚠️  Tor still bootstrapping, may take a minute..."
fi

# Start stunnel
systemctl enable helloworld-stunnel
systemctl restart helloworld-stunnel

# Start rotator
systemctl enable helloworld-tor-rotate
systemctl start helloworld-tor-rotate

echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""
echo "Architecture:"
echo "  Client → TLS(443) → SSH → Server"
echo "                         ↓"
echo "  Your apps use Tor SOCKS (127.0.0.1:9050)"
echo "                         ↓"
echo "  Traffic exits via Tor (rotating IP)"
echo ""
echo "Next steps:"
echo ""
echo "  1. Add your SSH key:"
echo "     echo 'YOUR_KEY' >> ~/.ssh/authorized_keys"
echo ""
echo "  2. Check status:"
echo "     helloworld-status"
echo ""
echo "  3. Test Tor exit IP:"
echo "     curl --socks5 127.0.0.1:9050 https://api.ipify.org"
echo ""
echo "========================================"
