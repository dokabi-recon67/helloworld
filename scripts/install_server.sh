#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "========================================"
echo "   HelloWorld Server Installer"
echo "========================================"
echo -e "${NC}"

if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Please run as root: sudo $0${NC}"
    exit 1
fi

echo -e "${YELLOW}[1/6] Detecting OS...${NC}"
if [ -f /etc/debian_version ]; then
    PKG_MGR="apt-get"
    PKG_UPDATE="apt-get update"
    STUNNEL_PKG="stunnel4"
elif [ -f /etc/redhat-release ]; then
    PKG_MGR="yum"
    PKG_UPDATE="yum check-update || true"
    STUNNEL_PKG="stunnel"
else
    echo -e "${RED}Unsupported OS. Please install manually.${NC}"
    exit 1
fi

echo -e "${YELLOW}[2/6] Installing dependencies...${NC}"
$PKG_UPDATE
$PKG_MGR install -y $STUNNEL_PKG openssh-server curl openssl tar gcc cmake make

echo -e "${YELLOW}[3/6] Downloading HelloWorld Server...${NC}"
cd /tmp
rm -rf helloworld-server
mkdir helloworld-server
cd helloworld-server

RELEASE_URL="https://github.com/yourusername/helloworld/releases/latest/download/helloworld-server.tar.gz"
if curl -fsSL "$RELEASE_URL" -o helloworld-server.tar.gz 2>/dev/null; then
    tar xzf helloworld-server.tar.gz
else
    echo -e "${YELLOW}Downloading from source...${NC}"
    curl -fsSL "https://github.com/yourusername/helloworld/archive/main.tar.gz" -o main.tar.gz
    tar xzf main.tar.gz
    mv helloworld-main/server/* .
fi

echo -e "${YELLOW}[4/6] Building...${NC}"
mkdir -p build
cd build
cmake ..
make
make install
cd ..

echo -e "${YELLOW}[5/6] Configuring...${NC}"
mkdir -p /etc/helloworld

if [ ! -f /etc/helloworld/server.pem ]; then
    openssl req -new -x509 -days 3650 -nodes \
        -out /etc/helloworld/server.pem \
        -keyout /etc/helloworld/server.key \
        -subj "/CN=$(hostname)" 2>/dev/null
fi

cat > /etc/helloworld/stunnel.conf << 'EOF'
pid = /var/run/stunnel-helloworld.pid
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key

[ssh]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
EOF

chmod 600 /etc/helloworld/server.key

echo -e "${YELLOW}[6/6] Opening firewall...${NC}"
if command -v ufw &> /dev/null; then
    ufw allow 443/tcp
elif command -v firewall-cmd &> /dev/null; then
    firewall-cmd --permanent --add-port=443/tcp
    firewall-cmd --reload
fi

echo 1 > /proc/sys/net/ipv4/ip_forward
echo "net.ipv4.ip_forward = 1" >> /etc/sysctl.conf

rm -rf /tmp/helloworld-server

echo
echo -e "${GREEN}========================================"
echo "   Installation Complete!"
echo "========================================${NC}"
echo
echo -e "Next steps:"
echo -e "  1. Add your SSH public key:"
echo -e "     ${CYAN}echo 'your-key' >> ~/.ssh/authorized_keys${NC}"
echo
echo -e "  2. Start the server:"
echo -e "     ${CYAN}sudo helloworld-server start${NC}"
echo
echo -e "  3. Check status:"
echo -e "     ${CYAN}sudo helloworld-server status${NC}"
echo
echo -e "Your server IP: ${CYAN}$(curl -s http://api.ipify.org 2>/dev/null || hostname -I | awk '{print $1}')${NC}"
echo

