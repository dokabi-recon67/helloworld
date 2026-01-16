#!/bin/bash
#
# HelloWorld Server Installer
# Works on: Ubuntu, Debian, Oracle Linux, CentOS, RHEL, Fedora
#

set -e

echo ""
echo "========================================"
echo "   HelloWorld Server Installer"
echo "========================================"
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo bash install_server.sh"
    exit 1
fi

# Detect OS
echo "[1/7] Detecting OS..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    echo "  Detected: $PRETTY_NAME"
else
    echo "  Unknown OS, assuming Debian-based"
    OS="debian"
fi

# Install packages
echo "[2/7] Installing dependencies..."
case $OS in
    ubuntu|debian)
        apt-get update -qq
        apt-get install -y -qq stunnel4 openssh-server curl openssl net-tools
        ;;
    ol|centos|rhel|fedora|rocky|almalinux)
        if command -v dnf &> /dev/null; then
            dnf install -y epel-release 2>/dev/null || dnf install -y oracle-epel-release-el9 2>/dev/null || true
            dnf install -y stunnel openssh-server curl openssl net-tools nc
        else
            yum install -y epel-release 2>/dev/null || true
            yum install -y stunnel openssh-server curl openssl net-tools nc
        fi
        ;;
    *)
        echo "  Attempting apt-get..."
        apt-get update -qq && apt-get install -y -qq stunnel4 openssh-server curl openssl
        ;;
esac

# Create directories
echo "[3/7] Creating directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4 2>/dev/null || mkdir -p /var/run/stunnel
chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || chown nobody:nobody /var/run/stunnel 2>/dev/null || true

# Generate TLS certificate
echo "[4/7] Generating TLS certificate..."
if [ ! -f /etc/helloworld/server.pem ]; then
    openssl req -x509 -nodes -days 3650 -newkey rsa:4096 \
        -keyout /etc/helloworld/server.key \
        -out /etc/helloworld/server.pem \
        -subj "/CN=cloudserver" 2>/dev/null
    chmod 600 /etc/helloworld/server.key
    chmod 644 /etc/helloworld/server.pem
    echo "  Certificate generated (valid for 10 years)"
else
    echo "  Certificate already exists, skipping"
fi

# Create stunnel config
echo "[5/7] Configuring stunnel..."
cat > /etc/helloworld/stunnel.conf << 'STUNNELCONF'
; HelloWorld Stunnel Configuration
; Wraps SSH in TLS on port 443

pid = /var/run/stunnel4/stunnel.pid
setuid = stunnel4
setgid = stunnel4

; Logging
debug = 4
output = /var/log/stunnel.log

; TLS settings
sslVersion = TLSv1.2
options = NO_SSLv2
options = NO_SSLv3

; SSH tunnel - accepts TLS on 443, forwards to SSH on 22
[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key
STUNNELCONF

# Fix config for different distros
if [ -d /var/run/stunnel ]; then
    sed -i 's|/var/run/stunnel4|/var/run/stunnel|g' /etc/helloworld/stunnel.conf
fi

# Check if stunnel user exists, adjust config if not
if ! id "stunnel4" &>/dev/null; then
    if id "nobody" &>/dev/null; then
        sed -i 's/stunnel4/nobody/g' /etc/helloworld/stunnel.conf
    fi
fi

# Link config
ln -sf /etc/helloworld/stunnel.conf /etc/stunnel/stunnel.conf 2>/dev/null || true

# Configure SSH
echo "[6/7] Configuring SSH..."
sed -i 's/#PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
sed -i 's/PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
systemctl restart sshd 2>/dev/null || systemctl restart ssh 2>/dev/null || service ssh restart

# Enable and start stunnel
echo "[7/7] Starting services..."

# For Debian/Ubuntu
if [ -f /etc/default/stunnel4 ]; then
    sed -i 's/ENABLED=0/ENABLED=1/' /etc/default/stunnel4
    systemctl restart stunnel4 2>/dev/null || service stunnel4 restart
    systemctl enable stunnel4 2>/dev/null || true
fi

# For RHEL/CentOS/Oracle Linux
if command -v systemctl &> /dev/null; then
    # Create systemd service if it doesn't exist
    if [ ! -f /etc/systemd/system/helloworld.service ] && [ ! -f /usr/lib/systemd/system/stunnel.service ]; then
        cat > /etc/systemd/system/helloworld.service << 'SERVICECONF'
[Unit]
Description=HelloWorld Tunnel Server (stunnel)
After=network.target sshd.service

[Service]
Type=forking
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure

[Install]
WantedBy=multi-user.target
SERVICECONF
        systemctl daemon-reload
        systemctl enable helloworld
        systemctl start helloworld
    else
        systemctl restart stunnel 2>/dev/null || systemctl restart stunnel4 2>/dev/null || true
        systemctl enable stunnel 2>/dev/null || systemctl enable stunnel4 2>/dev/null || true
    fi
fi

# Create helper scripts
echo "Creating helper scripts..."

# Status script
cat > /usr/local/bin/helloworld-status << 'STATUSSCRIPT'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld Server Status"
echo "========================================"
echo ""
echo "Services:"
echo "  Stunnel:  $(systemctl is-active stunnel4 2>/dev/null || systemctl is-active helloworld 2>/dev/null || systemctl is-active stunnel 2>/dev/null || echo 'unknown')"
echo "  SSH:      $(systemctl is-active sshd 2>/dev/null || systemctl is-active ssh 2>/dev/null || echo 'unknown')"
echo ""
echo "Port 443:"
ss -tlnp 2>/dev/null | grep ':443' || netstat -tlnp 2>/dev/null | grep ':443' || echo "  Not detected"
echo ""
echo "Public IP:"
echo "  $(curl -s --max-time 5 https://api.ipify.org 2>/dev/null || echo 'Could not fetch')"
echo ""
echo "========================================"
STATUSSCRIPT
chmod +x /usr/local/bin/helloworld-status

# Get public IP
PUBLIC_IP=$(curl -s --max-time 10 https://api.ipify.org 2>/dev/null || echo "Unable to detect")

# Verify installation
echo ""
echo "========================================"
echo "   Installation Complete!"
echo "========================================"
echo ""
echo "Server IP:     $PUBLIC_IP"
echo "Server Port:   443"
echo ""

# Check if stunnel is running
if ss -tlnp 2>/dev/null | grep -q ':443' || netstat -tlnp 2>/dev/null | grep -q ':443'; then
    echo "Status:        RUNNING (port 443 is open)"
else
    echo "Status:        WARNING - port 443 may not be listening"
    echo "               Try: sudo systemctl restart stunnel4"
fi

echo ""
echo "Next steps:"
echo "  1. Download HelloWorld client on your PC/Mac"
echo "  2. Add this server using IP: $PUBLIC_IP"
echo "  3. Port: 443"
echo "  4. Username: $(whoami) or the user you SSH with"
echo ""
echo "Commands:"
echo "  helloworld-status    - Check server status"
echo ""
echo "========================================"
