# PowerShell script to remove all source code, keep only binaries
# WARNING: This is destructive! Make sure you have a backup.

Write-Host "========================================" -ForegroundColor Red
Write-Host "REMOVING SOURCE CODE" -ForegroundColor Red
Write-Host "========================================" -ForegroundColor Red
Write-Host ""
Write-Host "This will DELETE all source code files!" -ForegroundColor Yellow
Write-Host "Only binaries and essential files will remain." -ForegroundColor Yellow
Write-Host ""

$confirm = Read-Host "Type 'DELETE SOURCE' to confirm"
if ($confirm -ne "DELETE SOURCE") {
    Write-Host "Cancelled." -ForegroundColor Green
    exit 0
}

Write-Host ""
Write-Host "Removing source code..." -ForegroundColor Yellow

# Remove source directories
$dirsToRemove = @(
    "client\src",
    "client\include",
    "server\src",
    "server\include",
    "client\build",
    "server\build",
    "build",
    "docs",
    "website\static",
    "website\js",
    "website\css"
)

foreach ($dir in $dirsToRemove) {
    if (Test-Path $dir) {
        Remove-Item -Path $dir -Recurse -Force
        Write-Host "  Removed: $dir" -ForegroundColor Green
    }
}

# Remove source files
$filesToRemove = @(
    "client\CMakeLists.txt",
    "server\CMakeLists.txt",
    "CMakeLists.txt",
    "*.c",
    "*.cpp",
    "*.h",
    "*.hpp",
    "*.m",
    "*.mm",
    "*.rc",
    "*.cmake",
    "requirements.txt",
    "report.md",
    "STEALTH_TUNNEL_ARCHITECTURE.md",
    "TODO_IMPLEMENTATION.md",
    "IMPLEMENTATION_SUMMARY.md"
)

foreach ($pattern in $filesToRemove) {
    Get-ChildItem -Path . -Filter $pattern -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force
    Write-Host "  Removed files matching: $pattern" -ForegroundColor Green
}

# Keep only essential files
Write-Host ""
Write-Host "Keeping essential files..." -ForegroundColor Yellow
Write-Host "  ✓ binaries/ (binaries and scripts)" -ForegroundColor Green
Write-Host "  ✓ LICENSE" -ForegroundColor Green
Write-Host "  ✓ .gitignore" -ForegroundColor Green
Write-Host "  ✓ README.md (if exists)" -ForegroundColor Green

# Update .gitignore to only ignore user data
@"
# User data (never commit)
config.txt
*.key
*.pem
*.log
*.conf
%APPDATA%/HelloWorld/
~/.helloworld/

# IDE
.idea/
.vscode/

# OS
.DS_Store
Thumbs.db
"@ | Out-File -FilePath ".gitignore" -Encoding UTF8

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Source code removed!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Remaining structure:" -ForegroundColor Yellow
Get-ChildItem -Recurse -Directory | Select-Object FullName | Format-Table -AutoSize
Write-Host ""
Write-Host "Next: git add -A && git commit -m 'Remove source code, binaries-only'" -ForegroundColor Cyan

