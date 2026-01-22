# HelloWorld Release Preparation
# This script prepares binaries for GitHub Releases

Write-Host '=== Building Release Binaries ===' -ForegroundColor Cyan

# Build Windows
cd client\build
cmake --build . --config Release
Copy-Item Release\helloworld.exe ..\..\release\helloworld-windows-x64.exe

# Build macOS (if on Mac)
# cd client\build
# cmake --build . --config Release
# Copy-Item Release\helloworld.app ..\..\release\helloworld-macos-x64.app

Write-Host 'âœ… Binaries ready in release\ folder' -ForegroundColor Green
Write-Host ''
Write-Host 'Next steps:'
Write-Host '1. Test the binaries'
Write-Host '2. Create GitHub Release'
Write-Host '3. Upload binaries to Release'
