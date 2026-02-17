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
cmake -S . -B . -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . -j
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Done!
echo.
echo Run the client with:
echo   .\build\Debug\mmorp_client.exe --http-url http://192.168.30.254:8080/ --ws-url ws://192.168.30.254:8080/v1/world/ws
echo.
.\build\Debug\mmorp_client.exe --http-url http://192.168.30.254:8080/ --ws-url ws://192.168.30.254:8080/v1/world/ws
pause
