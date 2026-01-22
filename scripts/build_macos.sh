#!/bin/bash
set -e

echo "========================================"
echo "HelloWorld macOS Build Script"
echo "========================================"
echo

cd "$(dirname "$0")/.."

echo "[1/5] Creating directories..."
mkdir -p build dist

echo "[2/5] Building client..."
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
cd ..

echo "[3/5] Creating app bundle..."
APP_DIR="dist/HelloWorld.app"
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

cp build/helloworld "$APP_DIR/Contents/MacOS/HelloWorld"

cat > "$APP_DIR/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>HelloWorld</string>
    <key>CFBundleIdentifier</key>
    <string>com.helloworld.client</string>
    <key>CFBundleName</key>
    <string>HelloWorld</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>MIT License</string>
</dict>
</plist>
EOF

echo "[4/5] Creating DMG..."
DMG_NAME="HelloWorld-macOS"
rm -f "dist/$DMG_NAME.dmg"

mkdir -p dist/dmg-temp
cp -r "$APP_DIR" dist/dmg-temp/
ln -s /Applications dist/dmg-temp/Applications

hdiutil create -volname "HelloWorld" -srcfolder dist/dmg-temp -ov -format UDZO "dist/$DMG_NAME.dmg"

rm -rf dist/dmg-temp

echo "[5/5] Done!"
echo
echo "Output:"
echo "  App Bundle: dist/HelloWorld.app"
echo "  DMG: dist/$DMG_NAME.dmg"
echo

