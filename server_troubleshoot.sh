#!/bin/bash
# Complete HelloWorld Server Troubleshooting Script
# Run this on your server to fix everything

echo "=========================================="
echo "  HelloWorld Server Troubleshooting"
echo "=========================================="
echo ""

# Step 1: Check current status
echo "[1/8] Checking current status..."
echo ""
sudo systemctl status helloworld
echo ""
echo "Press Enter to continue..."
read

# Step 2: Stop everything
echo "[2/8] Stopping all stunnel processes..."
sudo systemctl stop helloworld
sudo pkill -9 stunnel
sleep 2
echo "Stopped"
echo ""

# Step 3: Check if port 443 is free
echo "[3/8] Checking if port 443 is free..."
sudo ss -tlnp | grep :443 || echo "Port 443 is free"
echo ""

# Step 4: Verify config file exists
echo "[4/8] Checking config file..."
if [ -f /etc/helloworld/stunnel.conf ]; then
    echo "Config file exists"
    echo "Current config:"
    cat /etc/helloworld/stunnel.conf
else
    echo "ERROR: Config file missing!"
    exit 1
fi
echo ""

# Step 5: Fix config file
echo "[5/8] Updating stunnel config with fixes..."
sudo bash -c 'cat > /etc/helloworld/stunnel.conf << "EOF"
; HelloWorld Stunnel Configuration
pid = /var/run/stunnel4/stunnel.pid
setuid = stunnel4
setgid = stunnel4
debug = 0
output = /var/log/helloworld-stunnel.log
sslVersion = all
options = NO_SSLv2
options = NO_SSLv3
ciphers = HIGH:!aNULL:!MD5:!RC4
verifyChain = no
verifyPeer = no
[ssh-tunnel]
accept = 0.0.0.0:443
connect = 127.0.0.1:22
cert = /etc/helloworld/server.pem
key = /etc/helloworld/server.key
EOF'
echo "Config updated"
echo ""

# Step 6: Verify certificates exist
echo "[6/8] Checking certificates..."
if [ -f /etc/helloworld/server.pem ] && [ -f /etc/helloworld/server.key ]; then
    echo "Certificates exist"
    ls -lh /etc/helloworld/server.*
else
    echo "ERROR: Certificates missing! Generating..."
    sudo openssl req -x509 -nodes -days 3650 -newkey rsa:4096 \
        -keyout /etc/helloworld/server.key \
        -out /etc/helloworld/server.pem \
        -subj "/CN=cloudserver" 2>/dev/null
    sudo chmod 600 /etc/helloworld/server.key
    sudo chmod 644 /etc/helloworld/server.pem
    echo "Certificates generated"
fi
echo ""

# Step 7: Restart service
echo "[7/8] Restarting service..."
sudo systemctl daemon-reload
sudo systemctl restart helloworld
sleep 3
echo "Service restarted"
echo ""

# Step 8: Verify it's working
echo "[8/8] Verifying service..."
echo ""
echo "Service status:"
sudo systemctl status helloworld --no-pager -l
echo ""
echo "Port 443 listening:"
sudo ss -tlnp | grep :443 || echo "ERROR: Port 443 not listening!"
echo ""
echo "Recent logs:"
sudo tail -n 20 /var/log/helloworld-stunnel.log 2>/dev/null || echo "No logs yet"
echo ""
echo "=========================================="
echo "  Troubleshooting Complete!"
echo "=========================================="

