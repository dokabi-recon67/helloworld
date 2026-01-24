# Building HelloWorld for macOS

## Quick Build

Since we're on Windows, the macOS binary must be built on a Mac. Here are the instructions:

### Prerequisites

- macOS 11+ (Big Sur or later)
- Xcode Command Line Tools
- CMake 3.16+

### Install Prerequisites

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake (if not already installed)
brew install cmake
```

### Build Steps

1. **Clone the repository** (if you haven't already):
   ```bash
   git clone https://github.com/dokabi-recon67/helloworld.git
   cd helloworld
   ```

2. **Build the client**:
   ```bash
   cd client
   mkdir build && cd build
   cmake ..
   make -j$(sysctl -n hw.ncpu)
   ```

3. **The binary will be created as**:
   - `build/helloworld.app` (macOS app bundle)

4. **Copy to binaries directory**:
   ```bash
   cp -r helloworld.app ../../binaries/macos/
   ```

5. **Commit and push**:
   ```bash
   git add binaries/macos/helloworld.app
   git commit -m "Add macOS binary"
   git push
   ```

## Alternative: Using the Build Script

A build script is available at `scripts/build_macos.sh`:

```bash
cd scripts
chmod +x build_macos.sh
./build_macos.sh
```

This will create:
- `dist/HelloWorld.app` - The app bundle
- `dist/HelloWorld-macOS.dmg` - A DMG installer (optional)

## Running the Binary

After building:

1. **Double-click** `helloworld.app` in Finder, OR
2. **Run from terminal**:
   ```bash
   open helloworld.app
   ```
3. **Or run directly**:
   ```bash
   ./helloworld.app/Contents/MacOS/helloworld
   ```

## Notes

- The macOS binary is a `.app` bundle (directory structure)
- It requires macOS 11+ due to API usage
- The binary is universal (works on Intel and Apple Silicon)
- No code signing required for personal use
- For distribution, you may want to code sign the app

## Troubleshooting

### "cmake: command not found"
Install CMake:
```bash
brew install cmake
```

### "xcode-select: error: command line tools are already installed"
This is fine - you already have Xcode Command Line Tools.

### Build fails with linker errors
Make sure you have Xcode Command Line Tools:
```bash
xcode-select --install
```

### App won't open (Gatekeeper)
If macOS blocks the app:
1. Right-click the app
2. Select "Open"
3. Click "Open" in the dialog

Or disable Gatekeeper (not recommended):
```bash
sudo spctl --master-disable
```

