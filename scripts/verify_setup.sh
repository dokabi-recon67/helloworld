#!/bin/bash
# HelloWorld Server Verification Script
# Run this on your VM to verify everything is set up correctly

echo "========================================"
echo "   HelloWorld Server Verification"
echo "========================================"
echo ""

# Check 1: Service status
echo "[1/6] Checking service status..."
if systemctl is-active --quiet helloworld; then
    echo "✅ Service: RUNNING"
else
    echo "❌ Service: NOT RUNNING"
    echo "   Run: sudo systemctl start helloworld"
fi
echo ""

# Check 2: Port 443 listening
echo "[2/6] Checking port 443..."
if ss -tlnp 2>/dev/null | grep -q ':443' || netstat -tlnp 2>/dev/null | grep -q ':443'; then
    echo "✅ Port 443: LISTENING"
    ss -tlnp 2>/dev/null | grep ':443' || netstat -tlnp 2>/dev/null | grep ':443'
else
    echo "❌ Port 443: NOT LISTENING"
    echo "   Run: sudo systemctl restart helloworld"
fi
echo ""

# Check 3: Public IP
echo "[3/6] Getting public IP..."
PUBLIC_IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
if [ -n "$PUBLIC_IP" ]; then
    echo "✅ Public IP: $PUBLIC_IP"
    echo "   Update this in your client config if it changed!"
else
    echo "⚠️  Could not fetch public IP"
fi
echo ""

# Check 4: SSH authorized_keys
echo "[4/6] Checking SSH authorized_keys..."
if [ -f ~/.ssh/authorized_keys ] && [ -s ~/.ssh/authorized_keys ]; then
    KEY_COUNT=$(wc -l < ~/.ssh/authorized_keys)
    echo "✅ SSH keys: $KEY_COUNT key(s) found"
    echo "   File: ~/.ssh/authorized_keys"
else
    echo "⚠️  SSH authorized_keys: Empty or missing"
    echo "   Add your public key to ~/.ssh/authorized_keys"
fi
echo ""

# Check 5: Stunnel config
echo "[5/6] Checking stunnel configuration..."
if [ -f /etc/helloworld/stunnel.conf ]; then
    echo "✅ Stunnel config: Found"
    if grep -q "accept = 0.0.0.0:443" /etc/helloworld/stunnel.conf; then
        echo "✅ Port 443 configured correctly"
    else
        echo "⚠️  Port 443 may not be configured correctly"
    fi
else
    echo "❌ Stunnel config: Missing"
    echo "   Run installer: curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash"
fi
echo ""

# Check 6: Recent stunnel logs
echo "[6/6] Checking recent stunnel logs..."
if [ -f /var/log/helloworld-stunnel.log ]; then
    echo "Last 5 log entries:"
    tail -n 5 /var/log/helloworld-stunnel.log 2>/dev/null | sed 's/^/   /'
    
    # Check for errors
    ERROR_COUNT=$(tail -n 50 /var/log/helloworld-stunnel.log 2>/dev/null | grep -i "error\|fail" | wc -l)
    if [ "$ERROR_COUNT" -gt 0 ]; then
        echo ""
        echo "⚠️  Found $ERROR_COUNT error(s) in recent logs"
    else
        echo ""
        echo "✅ No recent errors in logs"
    fi
else
    echo "⚠️  Log file not found (service may not have started yet)"
fi
echo ""

# Summary
echo "========================================"
echo "   Summary"
echo "========================================"
echo ""
echo "If everything shows ✅, your server is ready!"
echo ""
echo "Next steps:"
echo "1. Update client config with server IP: $PUBLIC_IP"
echo "2. Make sure GCP firewall allows port 443"
echo "3. Test connection from HelloWorld app"
echo ""
echo "To update to latest installer:"
echo "curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash"
echo ""

