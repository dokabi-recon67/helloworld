#!/bin/bash
# Quick update script for HelloWorld server
# Downloads latest installer and runs it

set -e

echo "========================================"
echo "   HelloWorld Server Update"
echo "========================================"
echo ""

# Get latest installer
echo "Downloading latest installer..."
curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh -o /tmp/helloworld_installer.sh

# Make executable
chmod +x /tmp/helloworld_installer.sh

# Run installer
echo "Running installer..."
sudo bash /tmp/helloworld_installer.sh

# Cleanup
rm -f /tmp/helloworld_installer.sh

echo ""
echo "========================================"
echo "   Update Complete!"
echo "========================================"
echo ""
echo "Run 'helloworld-status' to verify everything is working"

