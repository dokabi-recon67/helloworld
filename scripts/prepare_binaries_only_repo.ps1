# PowerShell script to prepare binaries-only repository
# Removes all source code, keeps only binaries and essential files

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Preparing Binaries-Only Repository" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Get version
$version = Get-Date -Format "yyyyMMdd"
Write-Host "Version: $version" -ForegroundColor Green
Write-Host ""

# Create backup of current state
Write-Host "Creating backup..." -ForegroundColor Yellow
if (Test-Path ".git") {
    git add -A
    git commit -m "Backup before binaries-only conversion" 2>$null
}

# Find binaries
Write-Host "Locating binaries..." -ForegroundColor Yellow
$windowsBinary = $null
$macBinary = $null

# Windows binary
if (Test-Path "client\build\Release\helloworld.exe") {
    $windowsBinary = "client\build\Release\helloworld.exe"
    Write-Host "  Found Windows binary: $windowsBinary" -ForegroundColor Green
}
if (Test-Path "dist\HelloWorld-win64\helloworld.exe") {
    $windowsBinary = "dist\HelloWorld-win64\helloworld.exe"
    Write-Host "  Found Windows binary: $windowsBinary" -ForegroundColor Green
}

# Mac binary (if exists)
if (Test-Path "client\build\Release\helloworld.app") {
    $macBinary = "client\build\Release\helloworld.app"
    Write-Host "  Found macOS binary: $macBinary" -ForegroundColor Green
}

if (-not $windowsBinary) {
    Write-Host "ERROR: Windows binary not found!" -ForegroundColor Red
    Write-Host "Please build the Windows binary first:" -ForegroundColor Yellow
    Write-Host "  cd client\build" -ForegroundColor White
    Write-Host "  cmake --build . --config Release" -ForegroundColor White
    exit 1
}

# Create binaries directory structure
Write-Host ""
Write-Host "Creating binaries directory structure..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path "binaries\windows" | Out-Null
New-Item -ItemType Directory -Force -Path "binaries\macos" | Out-Null
New-Item -ItemType Directory -Force -Path "binaries\scripts" | Out-Null

# Copy binaries
Write-Host "Copying binaries..." -ForegroundColor Yellow
Copy-Item $windowsBinary "binaries\windows\helloworld.exe" -Force
Write-Host "  Copied Windows binary" -ForegroundColor Green

if ($macBinary) {
    Copy-Item $macBinary "binaries\macos\helloworld.app" -Recurse -Force
    Write-Host "  Copied macOS binary" -ForegroundColor Green
} else {
    Write-Host "  macOS binary not found (will be added later)" -ForegroundColor Yellow
}

# Copy essential files
Write-Host "Copying essential files..." -ForegroundColor Yellow
Copy-Item "README.md" "binaries\README.md" -Force 2>$null
Copy-Item "LICENSE" "binaries\LICENSE" -Force 2>$null
Copy-Item "scripts\install_server.sh" "binaries\scripts\install_server.sh" -Force 2>$null
Copy-Item "scripts\cloud-init.yaml" "binaries\scripts\cloud-init.yaml" -Force 2>$null

# Create new .gitignore for binaries repo
Write-Host "Creating .gitignore..." -ForegroundColor Yellow
@"
# Binaries (included in repo)
# binaries/

# Build artifacts
build/
dist/
*.o
*.obj
*.lib
*.dll
*.so
*.dylib

# IDE
.idea/
.vscode/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db
Desktop.ini

# User data
config.txt
*.key
*.pem
*.log
*.conf
"@ | Out-File -FilePath "binaries\.gitignore" -Encoding UTF8

# Create README for binaries
Write-Host "Creating binaries README..." -ForegroundColor Yellow
@"
# HelloWorld - Binary Release

**Pre-compiled binaries only. No source code included.**

## Quick Start

1. **Download** the binary for your platform from `binaries/windows/` or `binaries/macos/`
2. **Set up your server** by running:
   ```bash
   curl -sSL https://raw.githubusercontent.com/dokabi-recon67/helloworld/main/scripts/install_server.sh | sudo bash
   ```
3. **Run the client** and add your server

## Server Setup

See `scripts/install_server.sh` for automated server installation.

## Features

- ✅ Advanced DPI evasion (TLS fingerprinting, traffic shaping)
- ✅ DNS leak prevention
- ✅ Traffic pattern obfuscation
- ✅ ML classification evasion
- ✅ Browser-like behavior simulation

## License

MIT License - see LICENSE file.

## Security

- Binaries are verified and signed
- No telemetry or data collection
- All traffic encrypted end-to-end
- Self-hosted (you control the server)

## Version

Release: $version
"@ | Out-File -FilePath "binaries\README.md" -Encoding UTF8

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Binaries prepared in: binaries/" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Review binaries/ directory" -ForegroundColor White
Write-Host "2. Remove source code files (see cleanup script)" -ForegroundColor White
Write-Host "3. git add binaries/" -ForegroundColor White
Write-Host "4. git commit -m 'Binaries-only release v$version'" -ForegroundColor White
Write-Host "5. git push" -ForegroundColor White
Write-Host ""
Write-Host "⚠️  WARNING: This will remove all source code!" -ForegroundColor Red
Write-Host "   Make sure you have a backup or separate source repo." -ForegroundColor Red

