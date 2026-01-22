#!/bin/bash
# Prepare binaries-only release for GitHub
# Removes all source code, keeps only binaries and essential files

set -e

RELEASE_DIR="HelloWorld-Binaries-Release"
VERSION=$(date +"%Y%m%d")

echo "=========================================="
echo "Preparing Binaries-Only Release"
echo "Version: $VERSION"
echo "=========================================="

# Create release directory
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

# Copy binaries
echo "Copying binaries..."
if [ -f "client/build/Release/helloworld.exe" ]; then
    mkdir -p "$RELEASE_DIR/bin"
    cp "client/build/Release/helloworld.exe" "$RELEASE_DIR/bin/"
    echo "  ✓ Windows binary copied"
fi

if [ -f "dist/HelloWorld-win64/helloworld.exe" ]; then
    mkdir -p "$RELEASE_DIR/bin"
    cp "dist/HelloWorld-win64/helloworld.exe" "$RELEASE_DIR/bin/" 2>/dev/null || true
fi

# Copy essential documentation (no source code references)
echo "Copying documentation..."
cp README.md "$RELEASE_DIR/" 2>/dev/null || true
cp LICENSE "$RELEASE_DIR/" 2>/dev/null || true

# Copy server installation script (users need this)
echo "Copying server scripts..."
mkdir -p "$RELEASE_DIR/scripts"
cp scripts/install_server.sh "$RELEASE_DIR/scripts/" 2>/dev/null || true
cp scripts/cloud-init.yaml "$RELEASE_DIR/scripts/" 2>/dev/null || true

# Create .gitignore for binaries repo
cat > "$RELEASE_DIR/.gitignore" << 'EOF'
# Binaries (already included)
*.exe
*.dll
*.so
*.dylib

# Build artifacts
build/
dist/
*.o
*.obj

# IDE
.idea/
.vscode/
*.swp

# OS
.DS_Store
Thumbs.db

# User data
config.txt
*.key
*.pem
*.log
EOF

# Create README for binaries repo
cat > "$RELEASE_DIR/README_BINARIES.md" << 'EOF'
# HelloWorld - Binary Release

This repository contains **pre-compiled binaries only**. Source code is not included.

## Quick Start

1. **Download the binary** for your platform from the `bin/` directory
2. **Set up your server** by running:
   ```bash
   curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
   ```
3. **Run the client** and add your server

## Server Setup

See `scripts/install_server.sh` for automated server installation.

## License

MIT License - see LICENSE file.

## Security

- Binaries are signed and verified
- No telemetry or data collection
- All traffic encrypted end-to-end
- Self-hosted (you control the server)

## Support

For issues or questions, please open an issue on the main repository.
EOF

echo ""
echo "=========================================="
echo "Release prepared in: $RELEASE_DIR"
echo "=========================================="
echo ""
echo "Next steps:"
echo "1. cd $RELEASE_DIR"
echo "2. git init"
echo "3. git add ."
echo "4. git commit -m 'Binary release v$VERSION'"
echo "5. git remote add origin <your-binaries-repo-url>"
echo "6. git push -u origin main"
echo ""
echo "⚠️  IMPORTANT: This directory contains NO source code"
echo "   Only binaries and essential files for distribution"

