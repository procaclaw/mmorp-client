@echo off
echo === MMORPG Client Build Script ===
echo.

echo [1/3] Pulling latest changes...
git pull
if errorlevel 1 (
    echo ERROR: Git pull failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Building...
if not exist "build" mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Done!
echo.
echo Run the client with:
echo   .\build\Release\mmorp_client.exe --http-url http://YOUR_SERVER:8080 --ws-url ws://YOUR_SERVER:8080/v1/world/ws
echo.
pause
