@echo off
echo ========================================
echo HelloWorld Windows Build Script
echo ========================================
echo.

cd /d "%~dp0\.."

echo [1/5] Creating directories...
if not exist "build" mkdir build
if not exist "dist" mkdir dist
if not exist "deps\stunnel" mkdir deps\stunnel

echo [2/5] Downloading stunnel...
if not exist "deps\stunnel\stunnel.exe" (
    echo Downloading stunnel...
    powershell -Command "Invoke-WebRequest -Uri 'https://www.stunnel.org/downloads/stunnel-5.71-win64-installer.exe' -OutFile 'deps\stunnel-installer.exe'"
    echo Please install stunnel and copy stunnel.exe to deps\stunnel\
    pause
)

echo [3/5] Building client...
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

cd ..

echo [4/5] Creating installer...
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    "C:\Program Files (x86)\NSIS\makensis.exe" scripts\installer_win.nsi
) else (
    echo NSIS not found. Please install NSIS to create installer.
    echo Download from: https://nsis.sourceforge.io/Download
    echo.
    echo Creating ZIP package instead...
    powershell -Command "Compress-Archive -Path 'build\Release\helloworld.exe' -DestinationPath 'dist\HelloWorld-win64.zip' -Force"
)

echo [5/5] Done!
echo.
echo Output:
if exist "dist\HelloWorld-Setup.exe" (
    echo   Installer: dist\HelloWorld-Setup.exe
)
if exist "dist\HelloWorld-win64.zip" (
    echo   ZIP: dist\HelloWorld-win64.zip
)
echo.
pause

