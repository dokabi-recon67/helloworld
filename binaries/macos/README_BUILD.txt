macOS Binary Build Instructions
===============================

This directory should contain helloworld.app after building.

IMPORTANT: This is a binaries-only repository. To build the macOS binary,
you need access to the source code repository.

Build Steps:
------------

1. Get the source code:
   - Clone the source repository (if you have access)
   - Or use the source code from before binaries-only conversion

2. On macOS (11+ required):
   ```bash
   cd client
   mkdir build && cd build
   cmake ..
   make
   ```

3. Copy the binary:
   ```bash
   cp -r helloworld.app ../../binaries/macos/
   ```

4. Commit and push:
   ```bash
   git add binaries/macos/helloworld.app
   git commit -m "Add macOS binary"
   git push
   ```

Requirements:
-------------
- macOS 11+ (Big Sur or later)
- Xcode Command Line Tools (xcode-select --install)
- CMake 3.16+ (brew install cmake)

Running the Binary:
-------------------
- Double-click helloworld.app in Finder
- Or run: open helloworld.app
- Or run: ./helloworld.app/Contents/MacOS/helloworld

Note:
-----
The macOS binary is a .app bundle (directory structure).
It contains the executable and all necessary resources.

