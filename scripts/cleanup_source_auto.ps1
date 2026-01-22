# Automated cleanup - removes all source code without confirmation
# WARNING: This is destructive!

Write-Host "========================================" -ForegroundColor Red
Write-Host "REMOVING ALL SOURCE CODE" -ForegroundColor Red
Write-Host "========================================" -ForegroundColor Red
Write-Host ""

# Remove source directories
Write-Host "Removing source directories..." -ForegroundColor Yellow
$dirsToRemove = @(
    "client\src",
    "client\include",
    "client\build",
    "client\resources",
    "server\src",
    "server\include",
    "server\resources",
    "server\build",
    "build",
    "docs",
    "website",
    "deps",
    "dist"
)

foreach ($dir in $dirsToRemove) {
    if (Test-Path $dir) {
        Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  Removed: $dir" -ForegroundColor Green
    }
}

# Remove source files from root and subdirectories
Write-Host "Removing source files..." -ForegroundColor Yellow
$patterns = @("*.c", "*.cpp", "*.h", "*.hpp", "*.m", "*.mm", "*.rc", "*.cmake", "CMakeLists.txt")

foreach ($pattern in $patterns) {
    Get-ChildItem -Path . -Filter $pattern -Recurse -ErrorAction SilentlyContinue | 
        Where-Object { $_.FullName -notlike "*\binaries\*" } | 
        Remove-Item -Force -ErrorAction SilentlyContinue
    Write-Host "  Removed files matching: $pattern" -ForegroundColor Green
}

# Remove documentation files (keep only binaries README)
Write-Host "Removing documentation files..." -ForegroundColor Yellow
$docFiles = @(
    "report.md",
    "STEALTH_TUNNEL_ARCHITECTURE.md",
    "TODO_IMPLEMENTATION.md",
    "IMPLEMENTATION_SUMMARY.md",
    "RELEASE_NOTES.md",
    "requirements.txt",
    "SECURITY.md",
    "config.example.json",
    "server_troubleshoot.sh",
    "setup_gcp_firewall.ps1"
)

foreach ($file in $docFiles) {
    if (Test-Path $file) {
        Remove-Item -Path $file -Force -ErrorAction SilentlyContinue
        Write-Host "  Removed: $file" -ForegroundColor Green
    }
}

# Remove empty directories
Write-Host "Cleaning up empty directories..." -ForegroundColor Yellow
Get-ChildItem -Path . -Directory -Recurse -ErrorAction SilentlyContinue | 
    Where-Object { 
        $_.FullName -notlike "*\binaries\*" -and 
        $_.FullName -notlike "*\.git\*" -and
        (Get-ChildItem -Path $_.FullName -Recurse -ErrorAction SilentlyContinue | Measure-Object).Count -eq 0
    } | 
    Remove-Item -Force -ErrorAction SilentlyContinue

# Remove client and server directories if empty
if (Test-Path "client") {
    $clientFiles = Get-ChildItem -Path "client" -Recurse -ErrorAction SilentlyContinue
    if ($clientFiles.Count -eq 0) {
        Remove-Item -Path "client" -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  Removed empty: client/" -ForegroundColor Green
    }
}

if (Test-Path "server") {
    $serverFiles = Get-ChildItem -Path "server" -Recurse -ErrorAction SilentlyContinue
    if ($serverFiles.Count -eq 0) {
        Remove-Item -Path "server" -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  Removed empty: server/" -ForegroundColor Green
    }
}

# Remove scripts that are not needed (keep only install_server.sh in binaries)
Write-Host "Cleaning up scripts..." -ForegroundColor Yellow
$scriptsToKeep = @("install_server.sh", "cloud-init.yaml")
Get-ChildItem -Path "scripts" -File -ErrorAction SilentlyContinue | 
    Where-Object { $_.Name -notin $scriptsToKeep } | 
    Remove-Item -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Source code removed!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Remaining files:" -ForegroundColor Yellow
Get-ChildItem -File | Select-Object Name | Format-Table -AutoSize
Write-Host ""
Write-Host "Remaining directories:" -ForegroundColor Yellow
Get-ChildItem -Directory | Select-Object Name | Format-Table -AutoSize

