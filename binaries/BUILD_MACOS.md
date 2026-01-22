# Building macOS Binary

## Prerequisites

- macOS 11+ (Big Sur or later)
- Xcode Command Line Tools
- CMake 3.16+

## Build Steps

```bash
# Install dependencies (if needed)
xcode-select --install

# Navigate to client directory
cd client

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Binary will be at: build/helloworld.app
```

## Alternative: Build from Source Repo

If you have access to the source repository:

```bash
git clone <source-repo-url>
cd helloworld/client
mkdir build && cd build
cmake ..
make
cp -r helloworld.app ../../binaries/macos/
```

## Notes

- The macOS binary is a `.app` bundle (directory)
- To run: `open helloworld.app` or double-click in Finder
- The binary requires macOS 11+ due to API usage

