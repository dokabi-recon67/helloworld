#!/bin/bash
#
# HelloWorld Server v2 - Tor Edition
# Routes ALL tunnel traffic through Tor for rotating exit IPs
#
# Usage: curl -sSL <url> | sudo bash
#
# Architecture:
#   Client -> TLS (443) -> SSH -> Server -> Tor -> Internet
#   Exit IP = Tor exit node (rotates every 100 requests)
#

echo "========================================"
echo "   HelloWorld Server v2 - Tor Edition"
echo "========================================"
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash $0"
    exit 1
fi

# Clean up previous install (iptables rules block internet!)
echo "[0/9] Cleaning up previous install..."
systemctl stop helloworld-stunnel 2>/dev/null || true
systemctl stop helloworld-tor-rotate 2>/dev/null || true
systemctl stop redsocks 2>/dev/null || true
pkill -9 stunnel 2>/dev/null || true
pkill -9 redsocks 2>/dev/null || true
iptables -t nat -F REDSOCKS 2>/dev/null || true
iptables -t nat -D OUTPUT -j REDSOCKS 2>/dev/null || true
iptables -t nat -X REDSOCKS 2>/dev/null || true
fuser -k 443/tcp 2>/dev/null || true
echo "   Done"

if [ -f /etc/debian_version ]; then
    PKG_UPDATE="apt-get update"
    PKG_INSTALL="apt-get install -y"
else
    echo "This script requires Debian/Ubuntu"
    exit 1
fi

set -e

echo "[1/9] Updating packages..."
$PKG_UPDATE > /dev/null 2>&1

echo "[2/9] Installing stunnel4..."
$PKG_INSTALL stunnel4 > /dev/null 2>&1

echo "[3/9] Installing Tor..."
$PKG_INSTALL tor > /dev/null 2>&1

echo "[4/9] Installing redsocks..."
$PKG_INSTALL redsocks netcat-openbsd > /dev/null 2>&1

set +e

echo "[5/9] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true

echo "[6/9] Generating TLS certificates..."
openssl req -new -x509 -days 3650 -nodes \
    -out /etc/helloworld/server.pem \
    -keyout /etc/helloworld/server.key \
    -subj "/CN=helloworld-server" 2>/dev/null
chmod 600 /etc/helloworld/server.key
chown stunnel4:stunnel4 /etc/helloworld/server.pem /etc/helloworld/server.key 2>/dev/null || true

echo "[7/9] Configuring Tor..."
cat > /etc/tor/torrc << 'EOF'
RunAsDaemon 1
SocksPort 127.0.0.1:9050
ControlPort 127.0.0.1:9051
CookieAuthentication 0
MaxCircuitDirtiness 300
NewCircuitPeriod 30
CircuitBuildTimeout 15
NumEntryGuards 3
LearnCircuitBuildTimeout 1
ConnectionPadding 0
ReducedConnectionPadding 1
Log notice file /var/log/tor/notices.log
DataDirectory /var/lib/tor
EOF

mkdir -p /var/log/tor
chown debian-tor:debian-tor /var/log/tor 2>/dev/null || true

echo "[8/9] Configuring redsocks..."
cat > /etc/redsocks.conf << 'EOF'
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
EOF

echo "[9/9] Configuring stunnel..."
cat > /etc/helloworld/stunnel.conf << 'EOF'
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
TIMEOUTclose = 0
TIMEOUTconnect = 10
TIMEOUTidle = 3600
EOF

# --- iptables routing script ---
cat > /etc/helloworld/setup-tor-routing.sh << 'EOF'
#!/bin/bash
TOR_UID=$(id -u debian-tor 2>/dev/null || id -u tor 2>/dev/null || echo "")
iptables -t nat -N REDSOCKS 2>/dev/null || iptables -t nat -F REDSOCKS
iptables -t nat -A REDSOCKS -d 0.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 10.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 127.0.0.0/8 -j RETURN
iptables -t nat -A REDSOCKS -d 169.254.0.0/16 -j RETURN
iptables -t nat -A REDSOCKS -d 172.16.0.0/12 -j RETURN
iptables -t nat -A REDSOCKS -d 192.168.0.0/16 -j RETURN
iptables -t nat -A REDSOCKS -p tcp -j REDIRECT --to-ports 12345
if [ -n "$TOR_UID" ]; then
    iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
fi
iptables -t nat -A OUTPUT -m owner --uid-owner 0 -j RETURN
iptables -t nat -A OUTPUT -p tcp --dport 80 -j REDSOCKS
iptables -t nat -A OUTPUT -p tcp --dport 443 -j REDSOCKS
echo "Tor routing enabled"
EOF
chmod +x /etc/helloworld/setup-tor-routing.sh

# --- Tor rotator script ---
cat > /usr/local/bin/helloworld-tor-rotate << 'EOF'
#!/bin/bash
ROTATE_AFTER=100
LOG="/var/log/helloworld-tor-rotator.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1" | tee -a "$LOG"
}

rotate_circuit() {
    (echo authenticate '""'; echo signal newnym; echo quit) | nc 127.0.0.1 9051 > /dev/null 2>&1
    sleep 2
    NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://icanhazip.com 2>/dev/null || echo "rotating...")
    log "ROTATED - New Tor exit IP: $NEW_IP"
    iptables -t nat -Z REDSOCKS 2>/dev/null
}

log "========================================="
log "Tor Rotator Started (every $ROTATE_AFTER connections)"
log "========================================="

sleep 5
CURRENT_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 15 https://icanhazip.com 2>/dev/null || echo "connecting...")
log "Initial Tor exit IP: $CURRENT_IP"
iptables -t nat -Z REDSOCKS 2>/dev/null

while true; do
    CONN_COUNT=$(iptables -t nat -L REDSOCKS -v -n 2>/dev/null | grep "REDIRECT" | awk '{print $1}')
    CONN_COUNT=${CONN_COUNT:-0}
    if [ "$CONN_COUNT" -ge "$ROTATE_AFTER" ] 2>/dev/null; then
        log "Reached $CONN_COUNT connections (threshold: $ROTATE_AFTER)"
        rotate_circuit
    fi
    sleep 3
done
EOF
chmod +x /usr/local/bin/helloworld-tor-rotate

# --- systemd: stunnel ---
cat > /etc/systemd/system/helloworld-stunnel.service << 'EOF'
[Unit]
Description=HelloWorld Stunnel TLS Wrapper
After=network.target

[Service]
Type=forking
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
ExecStop=/bin/kill -TERM $MAINPID
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

# --- systemd: rotator ---
cat > /etc/systemd/system/helloworld-tor-rotate.service << 'EOF'
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
EOF

# --- systemd: routing ---
cat > /etc/systemd/system/helloworld-tor-routing.service << 'EOF'
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
EOF

# --- status command ---
cat > /usr/local/bin/helloworld-status << 'EOF'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld v2 - Tor Edition"
echo "========================================"
echo ""
echo "Services:"
ss -tlnp 2>/dev/null | grep -q ":443" && echo "  OK Stunnel (443)" || echo "  XX Stunnel"
ss -tlnp 2>/dev/null | grep -q ":22" && echo "  OK SSH (22)" || echo "  XX SSH"
ss -tlnp 2>/dev/null | grep -q ":9050" && echo "  OK Tor SOCKS (9050)" || echo "  XX Tor"
ss -tlnp 2>/dev/null | grep -q ":12345" && echo "  OK Redsocks (12345)" || echo "  XX Redsocks"
pgrep -f "helloworld-tor-rotate" > /dev/null && echo "  OK IP Rotator" || echo "  XX Rotator stopped"
echo ""
echo "========================================"
echo "IP Addresses"
echo "========================================"
SERVER_IP=$(curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip 2>/dev/null)
if [ -z "$SERVER_IP" ]; then
    SERVER_IP=$(curl -s --max-time 5 --noproxy '*' ifconfig.me 2>/dev/null || echo "unknown")
fi
TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://icanhazip.com 2>/dev/null || echo "error")
echo ""
echo "  SERVER IP: $SERVER_IP"
echo "  TOR EXIT:  $TOR_IP"
echo ""
if [ "$TOR_IP" != "error" ] && [ "$TOR_IP" != "$SERVER_IP" ]; then
    echo "  Tor routing ACTIVE - IPs are different!"
else
    echo "  Check Tor/redsocks status"
fi
echo ""
echo "Commands:"
echo "  helloworld-status    - This status"
echo "  helloworld-newip     - Force new Tor IP now"
echo ""
EOF
chmod +x /usr/local/bin/helloworld-status

# --- newip command ---
cat > /usr/local/bin/helloworld-newip << 'EOF'
#!/bin/bash
echo "Requesting new Tor circuit..."
(echo authenticate '""'; echo signal newnym; echo quit) | nc 127.0.0.1 9051 > /dev/null 2>&1
sleep 3
NEW_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://icanhazip.com)
echo "New Tor exit IP: $NEW_IP"
EOF
chmod +x /usr/local/bin/helloworld-newip

# =============================================
# START SERVICES
# =============================================

echo ""
echo "Starting services..."

systemctl daemon-reload

# 1) Tor
systemctl enable tor
systemctl restart tor
echo "Waiting for Tor to bootstrap (30s)..."
sleep 30

TOR_TEST=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://icanhazip.com 2>/dev/null)
if [ -z "$TOR_TEST" ]; then
    TOR_TEST=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://ifconfig.me 2>/dev/null)
fi
if [ -n "$TOR_TEST" ]; then
    echo "Tor connected! Exit: $TOR_TEST"
else
    echo "Tor still connecting..."
fi

# 2) Redsocks
systemctl enable redsocks 2>/dev/null || true
systemctl restart redsocks 2>/dev/null || redsocks -c /etc/redsocks.conf &
sleep 2

# 3) Tor routing
/etc/helloworld/setup-tor-routing.sh

# 4) Stunnel
pkill -9 stunnel 2>/dev/null || true
fuser -k 443/tcp 2>/dev/null || true
sleep 1
systemctl enable helloworld-stunnel
systemctl start helloworld-stunnel

# 5) Rotator
systemctl enable helloworld-tor-rotate
systemctl start helloworld-tor-rotate

# =============================================
# FINAL OUTPUT
# =============================================

sleep 2

echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""

SERVER_IP=$(curl -s -H "Metadata-Flavor: Google" http://metadata.google.internal/computeMetadata/v1/instance/network-interfaces/0/access-configs/0/external-ip 2>/dev/null)
if [ -z "$SERVER_IP" ]; then
    SERVER_IP=$(curl -s --max-time 5 --noproxy '*' ifconfig.me 2>/dev/null || echo "unknown")
fi

TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://icanhazip.com 2>/dev/null)
if [ -z "$TOR_IP" ]; then
    TOR_IP=$(curl -s --socks5 127.0.0.1:9050 --max-time 10 https://ifconfig.me 2>/dev/null || echo "connecting...")
fi

echo "Server IP: $SERVER_IP (use in client config)"
echo "Tor Exit:  $TOR_IP (your traffic appears here)"
echo ""
echo "Architecture:"
echo "  Client -> TLS(443) -> SSH -> Tor -> Internet"
echo ""
echo "Features:"
echo "  - Double encryption (TLS + SSH)"
echo "  - Traffic exits via Tor (not server IP)"
echo "  - IP rotates every 100 connections"
echo "  - Manual rotation: helloworld-newip"
echo ""
echo "Next steps:"
echo "  1. Add SSH key:"
echo "     mkdir -p ~/.ssh && chmod 700 ~/.ssh"
echo "     echo 'YOUR_KEY' >> ~/.ssh/authorized_keys"
echo "     chmod 600 ~/.ssh/authorized_keys"
echo ""
echo "  2. Check status: helloworld-status"
echo ""
echo "========================================"
