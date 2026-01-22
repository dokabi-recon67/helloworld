#!/bin/bash
# Build macOS binary for HelloWorld
# Run this on a macOS system

set -e

echo "========================================"
echo "Building macOS Binary"
echo "========================================"
echo ""

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "ERROR: This script must be run on macOS"
    exit 1
fi

# Check for Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: Xcode not found. Please install Xcode Command Line Tools:"
    echo "  xcode-select --install"
    exit 1
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install:"
    echo "  brew install cmake"
    exit 1
fi

echo "Prerequisites check passed!"
echo ""

# Note: This script assumes you have the source code
# Since we're in binaries-only repo, we'll create a placeholder
# and instructions for building

echo "========================================"
echo "macOS Binary Build Instructions"
echo "========================================"
echo ""
echo "Since this is a binaries-only repository, you need:"
echo ""
echo "1. Access to the source code repository"
echo "2. macOS 11+ (Big Sur or later)"
echo "3. Xcode Command Line Tools"
echo "4. CMake 3.16+"
echo ""
echo "Build steps:"
echo ""
echo "  git clone <source-repo-url>"
echo "  cd helloworld/client"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make"
echo "  cp -r helloworld.app ../../binaries/macos/"
echo ""
echo "Alternatively, if you have the source locally:"
echo ""
echo "  cd client"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make"
echo "  cp -r helloworld.app ../../binaries/macos/"
echo ""
echo "========================================"

# Create placeholder file
mkdir -p ../binaries/macos
cat > ../binaries/macos/README_BUILD.txt << 'EOF'
macOS Binary Build Instructions
===============================

This directory should contain helloworld.app after building.

To build:
1. Get source code from the source repository
2. On macOS, run:
   cd client
   mkdir build && cd build
   cmake ..
   make
   cp -r helloworld.app ../../binaries/macos/

Requirements:
- macOS 11+ (Big Sur or later)
- Xcode Command Line Tools
- CMake 3.16+

The binary will be a .app bundle that can be run by:
- Double-clicking in Finder
- Running: open helloworld.app
EOF

echo "Created placeholder README in binaries/macos/"
echo ""
echo "To build the actual binary, you need access to the source code."

