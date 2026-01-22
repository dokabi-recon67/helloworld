#!/bin/bash
# Quick VM Status Check Script
# Run this on your VM after restart to verify everything is working

echo "========================================"
echo "   HelloWorld VM Status Check"
echo "========================================"
echo ""

echo "[1/5] Checking current IP address..."
CURRENT_IP=$(curl -s https://api.ipify.org)
echo "  Current IP: $CURRENT_IP"
echo ""

echo "[2/5] Checking stunnel service..."
if systemctl is-active --quiet helloworld; then
    echo "  Status: ACTIVE ✓"
else
    echo "  Status: INACTIVE ✗"
    echo "  Attempting to start..."
    sudo systemctl start helloworld
    sleep 2
    if systemctl is-active --quiet helloworld; then
        echo "  Status: NOW ACTIVE ✓"
    else
        echo "  Status: FAILED TO START ✗"
    fi
fi
echo ""

echo "[3/5] Checking port 443..."
if sudo ss -tlnp | grep -q ":443"; then
    echo "  Port 443: LISTENING ✓"
    sudo ss -tlnp | grep ":443"
else
    echo "  Port 443: NOT LISTENING ✗"
fi
echo ""

echo "[4/5] Checking stunnel logs (last 10 lines)..."
if [ -f /var/log/helloworld-stunnel.log ]; then
    echo "  Recent log entries:"
    sudo tail -10 /var/log/helloworld-stunnel.log | sed 's/^/    /'
else
    echo "  No log file found"
fi
echo ""

echo "[5/5] Summary..."
echo "  IP Address: $CURRENT_IP"
echo "  Stunnel: $(systemctl is-active helloworld)"
echo "  Port 443: $(sudo ss -tlnp | grep -q ':443' && echo 'LISTENING' || echo 'NOT LISTENING')"
echo ""

echo "========================================"
echo "If IP changed, update client config with:"
echo "  host = $CURRENT_IP"
echo "========================================"

