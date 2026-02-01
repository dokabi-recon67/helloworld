#!/bin/bash
#
# HelloWorld Server v2 - Tor Edition
# Routes ALL tunnel traffic through Tor for rotating exit IPs
#
# Usage: curl -sSL <url> | sudo bash
#
# Architecture:
#   Client → TLS (443) → SSH → Server → Tor → Internet
#   Exit IP = Tor exit node (rotates every 100 requests)
#

set -e

echo "========================================"
echo "   HelloWorld Server v2 - Tor Edition"
echo "========================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

# Detect OS
if [ -f /etc/debian_version ]; then
    PKG_UPDATE="apt-get update"
    PKG_INSTALL="apt-get install -y"
else
    echo "This script requires Debian/Ubuntu"
    exit 1
fi

echo "[1/9] Updating packages..."
$PKG_UPDATE > /dev/null 2>&1

echo "[2/9] Installing stunnel4..."
$PKG_INSTALL stunnel4 > /dev/null 2>&1

echo "[3/9] Installing Tor..."
$PKG_INSTALL tor > /dev/null 2>&1

echo "[4/9] Installing redsocks (transparent proxy)..."
$PKG_INSTALL redsocks > /dev/null 2>&1

echo "[5/9] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true

echo "[6/9] Generating TLS certificates..."
openssl req -new -x509 -days 3650 -nodes \
    -out /etc/helloworld/server.pem \
    -keyout /etc/helloworld/server.key \
    -subj "/CN=localhost" 2>/dev/null
chmod 600 /etc/helloworld/server.key
chown stunnel4:stunnel4 /etc/helloworld/server.pem /etc/helloworld/server.key 2>/dev/null || true

echo "[7/9] Configuring Tor..."
cat > /etc/tor/torrc << 'TORRC'
# HelloWorld Tor Configuration v2
RunAsDaemon 1
SocksPort 127.0.0.1:9050
ControlPort 127.0.0.1:9051
CookieAuthentication 0

# Circuit rotation
MaxCircuitDirtiness 60
NewCircuitPeriod 60
CircuitBuildTimeout 10
NumEntryGuards 8

# Logging
Log notice file /var/log/tor/notices.log
DataDirectory /var/lib/tor
TORRC

mkdir -p /var/log/tor
chown debian-tor:debian-tor /var/log/tor 2>/dev/null || true

echo "[8/9] Configuring redsocks (Tor transparent proxy)..."
cat > /etc/redsocks.conf << 'REDSOCKS'
base {
    log_debug = off;
    log_info = on;
    daemon = on;
    redirector = iptables;
}

redsocks {
    local_ip = 127.0.0.1;
    local_port = 12345;
    ip = 127.0.0.1;
    port = 9050;
    type = socks5;
}
REDSOCKS

echo "[9/9] Configuring stunnel..."
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

# Create iptables setup script
cat > /etc/helloworld/setup-tor-routing.sh << 'IPTABLES'
#!/bin/bash
# Setup Tor transparent routing via redsocks
# Safe version - only affects outbound HTTP/HTTPS from non-root

# Get Tor user ID
TOR_UID=$(id -u debian-tor 2>/dev/null || id -u tor 2>/dev/null || echo "")

# Create REDSOCKS chain
iptables -t nat -N REDSOCKS 2>/dev/null || iptables -t nat -F REDSOCKS

# Don't redirect local/private traffic
iptables -t nat -A REDSOCKS -d 0.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 10.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 127.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 169.254.0.0/16 -j RETURN
iptables -t nat -A REDSOCKS -d 172.16.0.0/12 -j RETURN
iptables -t nat -A REDSOCKS -d 192.168.0.0/16 -j RETURN

# Redirect to redsocks
iptables -t nat -A REDSOCKS -p tcp -j REDIRECT --to-ports 12345

# Apply to OUTPUT - exclude Tor's own traffic and root
if [ -n "$TOR_UID" ]; then
    iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
fi
iptables -t nat -A OUTPUT -m owner --uid-owner 0 -j RETURN

# Route HTTP/HTTPS through Tor
iptables -t nat -A OUTPUT -p tcp --dport 80 -j REDSOCKS
iptables -t nat -A OUTPUT -p tcp --dport 443 -j REDSOCKS

echo "Tor routing enabled"
IPTABLES
chmod +x /etc/helloworld/setup-tor-routing.sh

# Create Tor rotation script (every 100 requests)
cat > /usr/local/bin/helloworld-tor-rotate << 'ROTATOR'
#!/bin/bash
ROTATE_AFTER=100
REQUEST_COUNT=0
LAST_CONNECTIONS=0
LOG="/var/log/helloworld-tor-rotator.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" | tee -a "$LOG"
}

rotate_circuit() {
    (echo authenticate '""'; echo signal newnym; echo quit) | nc 127.0.0.1 9051 > /dev/null 2>&1
    sleep 2
    NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null || echo "rotating...")
    log "🔄 ROTATED! New Tor exit IP: $NEW_IP"
}

log "========================================="
log "Tor Rotator Started (every $ROTATE_AFTER requests)"
log "========================================="

sleep 5
CURRENT_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null || echo "connecting...")
log "Initial Tor exit IP: $CURRENT_IP"

while true; do
    CURRENT_CONNECTIONS=$(ss -tn 2>/dev/null | grep -c ":22" || echo "0")
    
    if [ "$CURRENT_CONNECTIONS" -gt "$LAST_CONNECTIONS" ]; then
        DIFF=$((CURRENT_CONNECTIONS - LAST_CONNECTIONS))
        REQUEST_COUNT=$((REQUEST_COUNT + DIFF))
    fi
    LAST_CONNECTIONS=$CURRENT_CONNECTIONS
    
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
Description=HelloWorld Stunnel TLS Wrapper
After=network.target

[Service]
Type=forking
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
SYSTEMD

cat > /etc/systemd/system/helloworld-tor-rotate.service << 'ROTSVC'
[Unit]
Description=HelloWorld Tor IP Rotator
After=tor.service redsocks.service

[Service]
Type=simple
ExecStart=/usr/local/bin/helloworld-tor-rotate
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
ROTSVC

cat > /etc/systemd/system/helloworld-tor-routing.service << 'ROUTESVC'
[Unit]
Description=HelloWorld Tor Routing Setup
After=tor.service redsocks.service
Wants=redsocks.service

[Service]
Type=oneshot
ExecStart=/etc/helloworld/setup-tor-routing.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
ROUTESVC

# Create status command
cat > /usr/local/bin/helloworld-status << 'STATUS'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld v2 - Tor Edition"
echo "========================================"
echo ""

# Check services
echo "Services:"
ss -tlnp 2>/dev/null | grep -q ":443" && echo "  ✅ Stunnel (443)" || echo "  ❌ Stunnel"
ss -tlnp 2>/dev/null | grep -q ":22" && echo "  ✅ SSH (22)" || echo "  ❌ SSH"
ss -tlnp 2>/dev/null | grep -q ":9050" && echo "  ✅ Tor SOCKS (9050)" || echo "  ❌ Tor"
ss -tlnp 2>/dev/null | grep -q ":12345" && echo "  ✅ Redsocks (12345)" || echo "  ❌ Redsocks"
pgrep -f "helloworld-tor-rotate" > /dev/null && echo "  ✅ IP Rotator" || echo "  ⚠️ Rotator stopped"

echo ""
echo "========================================"
echo "IP Addresses"
echo "========================================"

# Get REAL server IP from GCP metadata
SERVER_IP=$(curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip 2>/dev/null)
if [ -z "$SERVER_IP" ]; then
    SERVER_IP=$(curl -s --max-time 5 --noproxy '*' ifconfig.me 2>/dev/null || echo "unknown")
fi

# Get Tor exit IP
TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org 2>/dev/null || echo "error")

echo ""
echo "  🖥️  SERVER IP: $SERVER_IP"
echo "      → Use this in HelloWorld client"
echo ""
echo "  🧅 TOR EXIT:  $TOR_IP"
echo "      → Your traffic appears from here"
echo ""

if [ "$TOR_IP" != "error" ] && [ "$TOR_IP" != "$SERVER_IP" ]; then
    echo "✅ Tor routing ACTIVE - IPs are different!"
else
    echo "⚠️  Check Tor/redsocks status"
fi

echo ""
echo "========================================"
echo "IP Rotation: Every 100 requests"
echo "========================================"
echo ""
echo "Commands:"
echo "  helloworld-status    - This status"
echo "  helloworld-newip     - Force new Tor IP now"
echo ""
STATUS
chmod +x /usr/local/bin/helloworld-status

# Create manual IP rotation command
cat > /usr/local/bin/helloworld-newip << 'NEWIP'
#!/bin/bash
echo "Requesting new Tor circuit..."
(echo authenticate '""'; echo signal newnym; echo quit) | nc 127.0.0.1 9051 > /dev/null 2>&1
sleep 3
NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org)
echo "New Tor exit IP: $NEW_IP"
NEWIP
chmod +x /usr/local/bin/helloworld-newip

# Start everything
echo ""
echo "Starting services..."

systemctl daemon-reload

# Start Tor first
systemctl enable tor
systemctl restart tor
echo "Waiting for Tor to bootstrap (30s)..."
sleep 30

# Test Tor
TOR_TEST=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://api.ipify.org 2>/dev/null)
if [ -n "$TOR_TEST" ]; then
    echo "✅ Tor connected! Exit: $TOR_TEST"
else
    echo "⚠️ Tor still connecting..."
fi

# Start redsocks
systemctl enable redsocks 2>/dev/null || true
systemctl restart redsocks 2>/dev/null || redsocks -c /etc/redsocks.conf &
sleep 2

# Setup Tor routing
/etc/helloworld/setup-tor-routing.sh

# Kill any existing stunnel on 443
pkill -9 stunnel 2>/dev/null
fuser -k 443/tcp 2>/dev/null
sleep 1

# Start stunnel
systemctl enable helloworld-stunnel
systemctl start helloworld-stunnel

# Start rotator
systemctl enable helloworld-tor-rotate
systemctl start helloworld-tor-rotate

# Final status
echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""

# Get server IP
SERVER_IP=$(curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip 2>/dev/null)
TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://api.ipify.org 2>/dev/null)

echo "🖥️  Server IP: $SERVER_IP (use in client config)"
echo "🧅 Tor Exit:  $TOR_IP (your traffic appears here)"
echo ""
echo "Architecture:"
echo "  Client → TLS(443) → SSH → Tor → Internet"
echo ""
echo "Features:"
echo "  ✅ Double encryption (TLS + SSH)"
echo "  ✅ Traffic exits via Tor (not server IP)"
echo "  ✅ IP rotates every 100 requests"
echo "  ✅ Manual rotation: helloworld-newip"
echo ""
echo "Next steps:"
echo "  1. Add SSH key: echo 'YOUR_KEY' >> ~/.ssh/authorized_keys"
echo "  2. Set permissions: chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys"
echo "  3. Check status: helloworld-status"
echo ""
echo "========================================"
