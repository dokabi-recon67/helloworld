#!/bin/bash
#
# HelloWorld Server Install Script
# Tested and working with Windows client - January 2026
#
# Usage: curl -sSL <url> | sudo bash
#        OR: sudo bash install_server.sh
#

set -e

echo "========================================"
echo "   HelloWorld Server Installation"
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
    echo "Unsupported OS. Please install stunnel4 and openssh-server manually."
    exit 1
fi

echo "[1/6] Updating package lists..."
$PKG_UPDATE

echo ""
echo "[2/6] Installing required packages..."
$PKG_INSTALL stunnel4 openssh-server openssl

echo ""
echo "[3/6] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true

echo ""
echo "[4/6] Generating TLS certificates..."
# Generate fresh self-signed certificate
openssl req -new -x509 -days 3650 -nodes \
    -out /etc/helloworld/server.pem \
    -keyout /etc/helloworld/server.key \
    -subj "/CN=helloworld-server" 2>/dev/null

chmod 600 /etc/helloworld/server.key
chown stunnel4:stunnel4 /etc/helloworld/server.pem /etc/helloworld/server.key 2>/dev/null || true

echo ""
echo "[5/6] Creating stunnel configuration..."
cat > /etc/helloworld/stunnel.conf << 'EOF'
; HelloWorld Server Configuration
; Auto-generated - works with Windows and Mac clients

pid = /var/run/stunnel4/stunnel.pid
setuid = stunnel4
setgid = stunnel4

; Logging
debug = 5
output = /var/log/helloworld-stunnel.log

[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key

; Connection settings
TIMEOUTclose = 0
TIMEOUTconnect = 10
TIMEOUTidle = 3600
EOF

echo ""
echo "[6/6] Starting stunnel service..."

# Stop any existing stunnel
pkill -9 stunnel 2>/dev/null || true
sleep 1

# Start stunnel
/usr/bin/stunnel /etc/helloworld/stunnel.conf

# Create systemd service for auto-start
cat > /etc/systemd/system/helloworld-stunnel.service << 'EOF'
[Unit]
Description=HelloWorld Stunnel Service
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

systemctl daemon-reload
systemctl enable helloworld-stunnel 2>/dev/null || true

# Create status command
cat > /usr/local/bin/helloworld-status << 'STATUSEOF'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld Server Status"
echo "========================================"
echo ""

# Check if stunnel is running
if pgrep -x stunnel > /dev/null; then
    echo "Service: active"
else
    echo "Service: INACTIVE (not running)"
fi

echo ""
echo "Port 443:"
if ss -tlnp | grep -q ":443"; then
    echo "  LISTENING (ready for connections)"
    ss -tlnp | grep ":443"
else
    echo "  NOT LISTENING"
fi

echo ""
echo "Stunnel logs (last 10 lines):"
tail -10 /var/log/helloworld-stunnel.log 2>/dev/null || echo "  No logs yet"

echo ""
echo "Public IP: $(curl -s https://api.ipify.org 2>/dev/null || echo 'Unable to determine')"
echo ""
echo "========================================"
STATUSEOF

chmod +x /usr/local/bin/helloworld-status

# Verify installation
echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""

sleep 2

if ss -tlnp | grep -q ":443"; then
    echo "✓ Stunnel is running on port 443"
else
    echo "✗ WARNING: Stunnel may not be running"
fi

PUBLIC_IP=$(curl -s https://api.ipify.org 2>/dev/null)
echo ""
echo "Server IP: $PUBLIC_IP"
echo ""
echo "Next steps:"
echo "1. Make sure port 443 is open in your firewall"
echo "2. Add your SSH public key to ~/.ssh/authorized_keys"
echo "3. Configure HelloWorld client with:"
echo "   - Host: $PUBLIC_IP"
echo "   - Port: 443"
echo "   - Username: $(whoami)"
echo ""
echo "Check status anytime: helloworld-status"
echo ""

