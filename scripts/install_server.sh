#!/bin/bash
#
# HelloWorld Server Installer
# One command setup - just run and go!
# Works on: Ubuntu, Debian, Oracle Linux, CentOS, RHEL, Fedora, Arch
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[OK]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[!]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

echo ""
echo "========================================"
echo "   HelloWorld Server Installer v1.2"
echo "========================================"
echo ""

# Check root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run as root:"
    echo "  sudo bash -c \"\$(curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh)\""
    exit 1
fi

# Detect OS
echo "[1/8] Detecting OS..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    print_status "Detected: $PRETTY_NAME"
else
    print_warning "Unknown OS, assuming Debian-based"
    OS="debian"
fi

# Install packages
echo ""
echo "[2/8] Installing dependencies..."
case $OS in
    ubuntu|debian|pop)
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        apt-get install -y -qq stunnel4 openssh-server curl openssl net-tools ufw >/dev/null 2>&1
        print_status "Installed: stunnel4, openssh-server, curl, openssl"
        ;;
    ol|centos|rhel|fedora|rocky|almalinux)
        if command -v dnf &> /dev/null; then
            dnf install -y -q epel-release 2>/dev/null || dnf install -y -q oracle-epel-release-el9 2>/dev/null || true
            dnf install -y -q stunnel openssh-server curl openssl net-tools firewalld nc >/dev/null 2>&1
        else
            yum install -y -q epel-release 2>/dev/null || true
            yum install -y -q stunnel openssh-server curl openssl net-tools firewalld nc >/dev/null 2>&1
        fi
        print_status "Installed: stunnel, openssh-server, curl, openssl"
        ;;
    arch|manjaro)
        pacman -Sy --noconfirm stunnel openssh curl openssl >/dev/null 2>&1
        print_status "Installed: stunnel, openssh, curl, openssl"
        ;;
    *)
        apt-get update -qq 2>/dev/null && apt-get install -y -qq stunnel4 openssh-server curl openssl >/dev/null 2>&1
        print_status "Installed packages"
        ;;
esac

# Create directories
echo ""
echo "[3/8] Setting up directories..."
mkdir -p /etc/helloworld
mkdir -p /var/run/stunnel4 2>/dev/null || mkdir -p /var/run/stunnel 2>/dev/null || true
mkdir -p /var/log

# Fix permissions for stunnel
if id "stunnel4" &>/dev/null; then
    chown stunnel4:stunnel4 /var/run/stunnel4 2>/dev/null || true
    STUNNEL_USER="stunnel4"
    STUNNEL_GROUP="stunnel4"
    STUNNEL_PID="/var/run/stunnel4/stunnel.pid"
elif id "stunnel" &>/dev/null; then
    chown stunnel:stunnel /var/run/stunnel 2>/dev/null || true
    STUNNEL_USER="stunnel"
    STUNNEL_GROUP="stunnel"
    STUNNEL_PID="/var/run/stunnel/stunnel.pid"
else
    STUNNEL_USER="nobody"
    STUNNEL_GROUP="nogroup"
    STUNNEL_PID="/var/run/stunnel.pid"
fi
print_status "Directories ready"

# Generate TLS certificate
echo ""
echo "[4/8] Generating TLS certificate..."
if [ ! -f /etc/helloworld/server.pem ] || [ ! -f /etc/helloworld/server.key ]; then
    openssl req -x509 -nodes -days 3650 -newkey rsa:4096 \
        -keyout /etc/helloworld/server.key \
        -out /etc/helloworld/server.pem \
        -subj "/CN=cloudserver" 2>/dev/null
    print_status "Certificate generated (valid 10 years)"
else
    print_status "Certificate already exists"
fi
chmod 600 /etc/helloworld/server.key
chmod 644 /etc/helloworld/server.pem
chown $STUNNEL_USER:$STUNNEL_GROUP /etc/helloworld/server.key /etc/helloworld/server.pem 2>/dev/null || true

# Create stunnel config
echo ""
echo "[5/8] Configuring stunnel..."
cat > /etc/helloworld/stunnel.conf << EOF
; HelloWorld Stunnel Configuration
; Wraps SSH in TLS on port 443

pid = $STUNNEL_PID
setuid = $STUNNEL_USER
setgid = $STUNNEL_GROUP

; Logging
debug = 0
output = /var/log/helloworld-stunnel.log

; TLS settings - compatible with all clients
sslVersion = all
options = NO_SSLv2
options = NO_SSLv3
options = CIPHER_SERVER_PREFERENCE
ciphers = ALL:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!SRP:!CAMELLIA
verifyChain = no
verifyPeer = no
renegotiation = no
TIMEOUTclose = 0

; SSH tunnel service
; Accepts TLS connections on port 443
; Forwards decrypted traffic to SSH on port 22
[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key
EOF

# Link config for systems that expect it elsewhere
ln -sf /etc/helloworld/stunnel.conf /etc/stunnel/stunnel.conf 2>/dev/null || true
print_status "Stunnel configured on port 443"

# Configure SSH for security
echo ""
echo "[6/8] Securing SSH..."
cp /etc/ssh/sshd_config /etc/ssh/sshd_config.backup 2>/dev/null || true
sed -i 's/#PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
sed -i 's/PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
sed -i 's/#PermitRootLogin.*/PermitRootLogin prohibit-password/' /etc/ssh/sshd_config

# Additional SSH hardening
echo "Protocol 2" >> /etc/ssh/sshd_config
echo "MaxAuthTries 3" >> /etc/ssh/sshd_config
echo "ClientAliveInterval 300" >> /etc/ssh/sshd_config
echo "ClientAliveCountMax 2" >> /etc/ssh/sshd_config
echo "PermitEmptyPasswords no" >> /etc/ssh/sshd_config
echo "X11Forwarding no" >> /etc/ssh/sshd_config
echo "PrintMotd no" >> /etc/ssh/sshd_config

# Disable root login if not already
if ! grep -q "^PermitRootLogin" /etc/ssh/sshd_config; then
    echo "PermitRootLogin prohibit-password" >> /etc/ssh/sshd_config
fi

systemctl restart sshd 2>/dev/null || systemctl restart ssh 2>/dev/null || service ssh restart 2>/dev/null || true
print_status "SSH secured (key-only authentication, hardened)"

# Open firewall port 443
echo ""
echo "[7/8] Configuring firewall..."

# Detect if running on Google Cloud Platform
IS_GCP=false
if curl -s -H "Metadata-Flavor: Google" --max-time 2 http://metadata.google.internal/computeMetadata/v1/instance/id >/dev/null 2>&1; then
    IS_GCP=true
    print_status "Detected: Google Cloud Platform"
fi

# Configure local firewall (UFW, Firewalld, or IPTables)
# UFW (Ubuntu/Debian)
if command -v ufw &> /dev/null; then
    ufw allow 443/tcp >/dev/null 2>&1 || true
    ufw allow 443/udp >/dev/null 2>&1 || true
    ufw allow 22/tcp >/dev/null 2>&1 || true
    print_status "UFW: port 443 opened"
fi
# Firewalld (RHEL/CentOS/Oracle)
if command -v firewall-cmd &> /dev/null; then
    systemctl start firewalld 2>/dev/null || true
    firewall-cmd --permanent --add-port=443/tcp >/dev/null 2>&1 || true
    firewall-cmd --permanent --add-port=443/udp >/dev/null 2>&1 || true
    firewall-cmd --permanent --add-service=ssh >/dev/null 2>&1 || true
    firewall-cmd --reload >/dev/null 2>&1 || true
    print_status "Firewalld: port 443 opened"
fi
# IPTables fallback
if command -v iptables &> /dev/null && ! command -v ufw &> /dev/null && ! command -v firewall-cmd &> /dev/null; then
    iptables -A INPUT -p tcp --dport 443 -j ACCEPT 2>/dev/null || true
    iptables -A INPUT -p udp --dport 443 -j ACCEPT 2>/dev/null || true
    print_status "IPTables: port 443 opened"
fi

# Configure Google Cloud Platform firewall (if on GCP)
if [ "$IS_GCP" = true ]; then
    echo ""
    echo "  Configuring GCP firewall..."
    
    # Try to configure GCP firewall using gcloud (if available)
    if command -v gcloud &> /dev/null; then
        PROJECT=$(gcloud config get-value project 2>/dev/null)
        if [ -n "$PROJECT" ]; then
            # Check if rule already exists
            if ! gcloud compute firewall-rules list --filter="name=allow-helloworld-443" --format="value(name)" 2>/dev/null | grep -q "allow-helloworld-443"; then
                if gcloud compute firewall-rules create allow-helloworld-443 \
                    --allow tcp:443 \
                    --source-ranges 0.0.0.0/0 \
                    --description "Allow HelloWorld stunnel on port 443" \
                    --direction INGRESS \
                    >/dev/null 2>&1; then
                    print_status "GCP firewall: rule created (port 443)"
                else
                    print_warning "GCP firewall: failed to create rule automatically"
                    echo "  You may need to create it manually in GCP Console"
                fi
            else
                print_status "GCP firewall: rule already exists"
            fi
        else
            print_warning "GCP firewall: gcloud not configured (no project set)"
            echo "  Run on your PC: gcloud config set project YOUR_PROJECT_ID"
        fi
    else
        print_warning "GCP firewall: gcloud CLI not installed on VM"
        echo "  To configure GCP firewall, run this on your PC:"
        echo "  gcloud compute firewall-rules create allow-helloworld-443 \\"
        echo "    --allow tcp:443 --source-ranges 0.0.0.0/0 \\"
        echo "    --description 'Allow HelloWorld stunnel on port 443'"
        echo ""
        echo "  Or create it in GCP Console:"
        echo "  https://console.cloud.google.com/compute/firewalls"
    fi
fi

# Create and start systemd service
echo ""
echo "[8/8] Starting HelloWorld service..."

# Create robust systemd service with auto-restart
cat > /etc/systemd/system/helloworld.service << EOF
[Unit]
Description=HelloWorld Tunnel Server
Documentation=https://github.com/dokabi-recon67/helloworld
After=network-online.target sshd.service
Wants=network-online.target

[Service]
Type=forking
PIDFile=$STUNNEL_PID
ExecStartPre=/bin/mkdir -p $(dirname $STUNNEL_PID)
ExecStartPre=/bin/chown $STUNNEL_USER:$STUNNEL_GROUP $(dirname $STUNNEL_PID)
ExecStart=/usr/bin/stunnel /etc/helloworld/stunnel.conf
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=5
StartLimitIntervalSec=60
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
EOF

# Stop any existing stunnel instances
systemctl stop stunnel4 2>/dev/null || true
systemctl stop stunnel 2>/dev/null || true
systemctl stop helloworld 2>/dev/null || true
pkill -f stunnel 2>/dev/null || true
sleep 1

# Enable and start our service
systemctl daemon-reload
systemctl enable helloworld >/dev/null 2>&1
systemctl start helloworld

# Also enable stunnel4 if it exists (Debian/Ubuntu)
if [ -f /etc/default/stunnel4 ]; then
    sed -i 's/ENABLED=0/ENABLED=1/' /etc/default/stunnel4
fi

# Wait for service to start
echo "  Waiting for service to start..."
sleep 3

# Verify service is running
MAX_RETRIES=5
RETRY_COUNT=0
SERVICE_UP=false

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if ss -tlnp 2>/dev/null | grep -q ':443' || netstat -tlnp 2>/dev/null | grep -q ':443'; then
        SERVICE_UP=true
        break
    fi
    RETRY_COUNT=$((RETRY_COUNT + 1))
    echo "  Retry $RETRY_COUNT/$MAX_RETRIES..."
    systemctl restart helloworld 2>/dev/null || true
    sleep 2
done

# Create helper scripts
cat > /usr/local/bin/helloworld-status << 'STATUSSCRIPT'
#!/bin/bash
echo ""
echo "========================================"
echo "   HelloWorld Server Status"
echo "========================================"
echo ""
echo "Service: $(systemctl is-active helloworld 2>/dev/null || echo 'checking...')"
echo ""
echo "Port 443:"
if ss -tlnp 2>/dev/null | grep -q ':443' || netstat -tlnp 2>/dev/null | grep -q ':443'; then
    echo "  LISTENING (ready for connections)"
    ss -tlnp 2>/dev/null | grep ':443' || netstat -tlnp 2>/dev/null | grep ':443'
else
    echo "  NOT LISTENING - try: sudo systemctl restart helloworld"
fi
echo ""
echo "Stunnel logs (last 10 lines):"
tail -n 10 /var/log/helloworld-stunnel.log 2>/dev/null || echo "  No logs found"
echo ""
echo "Public IP: $(curl -s --max-time 5 https://api.ipify.org 2>/dev/null || echo 'fetching...')"
echo ""
echo "========================================"
STATUSSCRIPT
chmod +x /usr/local/bin/helloworld-status

cat > /usr/local/bin/helloworld-restart << 'RESTARTSCRIPT'
#!/bin/bash
echo "Restarting HelloWorld..."
sudo systemctl restart helloworld
sleep 2
if ss -tlnp 2>/dev/null | grep -q ':443'; then
    echo "Service restarted successfully!"
else
    echo "Warning: Service may not be running. Check: sudo journalctl -u helloworld"
fi
RESTARTSCRIPT
chmod +x /usr/local/bin/helloworld-restart

# Get public IP
PUBLIC_IP=$(curl -s --max-time 10 https://api.ipify.org 2>/dev/null || curl -s --max-time 10 https://ifconfig.me 2>/dev/null || echo "Could not detect")

# Final status
echo ""
echo "========================================"
if [ "$SERVICE_UP" = true ]; then
    echo -e "   ${GREEN}Installation Complete!${NC}"
else
    echo -e "   ${YELLOW}Installation Complete (check status)${NC}"
fi
echo "========================================"
echo ""
echo "  Server IP:     $PUBLIC_IP"
echo "  Server Port:   443"
echo ""

if [ "$SERVICE_UP" = true ]; then
    echo -e "  Status:        ${GREEN}RUNNING${NC}"
else
    echo -e "  Status:        ${YELLOW}Check with: helloworld-status${NC}"
fi

echo ""
echo "  Commands:"
echo "    helloworld-status   - Check if server is running"
echo "    helloworld-restart  - Restart the server"
echo ""
echo "  Client Setup:"
echo "    1. Download HelloWorld app for Windows/Mac"
echo "    2. Click '+ ADD' to add server"
echo "    3. Host: $PUBLIC_IP"
echo "    4. Port: 443"
echo "    5. SSH Key: Your private key file"
echo "    6. Username: $(logname 2>/dev/null || echo 'your-ssh-user')"
echo ""
echo "========================================"
echo ""

# Log installation
echo "HelloWorld installed at $(date)" >> /var/log/helloworld-install.log
echo "Public IP: $PUBLIC_IP" >> /var/log/helloworld-install.log
