# Create HelloWorld Desktop Shortcut with Icon
# This script creates a desktop shortcut pointing to the latest HelloWorld executable

$ErrorActionPreference = "Stop"

$desktop = [Environment]::GetFolderPath("Desktop")
$exePath = "$PSScriptRoot\..\client\build\Release\helloworld.exe"
$shortcutPath = Join-Path $desktop "HelloWorld.lnk"

Write-Host "Creating HelloWorld desktop shortcut..."
Write-Host "  Target: $exePath"
Write-Host "  Shortcut: $shortcutPath"

if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: Executable not found at $exePath" -ForegroundColor Red
    Write-Host "Please build the project first: cd client && cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

# Remove old shortcut if exists
if (Test-Path $shortcutPath) {
    Remove-Item $shortcutPath -Force
    Write-Host "Removed old shortcut"
}

# Create shortcut using COM object
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $exePath
$shortcut.WorkingDirectory = Split-Path $exePath
$shortcut.Description = "HelloWorld - DPI-Resistant Encrypted Tunnel"
$shortcut.WindowStyle = 1  # Normal window

# Try to set icon from executable (Windows will extract it automatically)
$shortcut.IconLocation = "$exePath,0"

$shortcut.Save()

Write-Host "Shortcut created successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Note: If the icon doesn't appear immediately, try:" -ForegroundColor Yellow
Write-Host "  1. Refresh desktop (F5)" -ForegroundColor Yellow
Write-Host "  2. Restart Windows Explorer" -ForegroundColor Yellow
Write-Host "  3. Clear icon cache: ie4uinit.exe -show" -ForegroundColor Yellow

