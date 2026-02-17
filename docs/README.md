# MMORPG Client Documentation

This folder contains project documentation for the SFML/OpenGL MMORPG client in this repository.

## Project Overview

The client is a standalone desktop application written in C++17. It provides:

- Username/password authentication over HTTP and JWT token extraction
- Character selection
- Real-time world connection over WebSocket
- 3D placeholder rendering of local and remote players
- Keyboard movement and a third-person follow camera

Core implementation files:

- `src/GameClient.cpp`
- `src/HttpAuthClient.cpp`
- `src/WebSocketClient.cpp`
- `src/Renderer3D.cpp`
- `src/WorldState.hpp`

## Prerequisites

Required:

- CMake 3.16+
- C++17 compiler (GCC/Clang/MSVC)
- SFML 2.5+ (`graphics`, `window`, `system`)
- OpenGL development libraries
- Threading support (`pthread` on Linux; provided by toolchain on most platforms)
- Git (for CMake FetchContent dependencies)

Fetched automatically at configure time:

- Asio (`asio-1-30-2`)
- websocketpp (`0.8.2`)
- nlohmann/json (`v3.11.3`)
- cpp-httplib (`v0.16.3`)

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/mmorp_client
```

Expected local server endpoints:

- HTTP auth base: `http://localhost:8080`
- WebSocket world endpoint: `ws://localhost:8080/v1/world/ws`

## Documentation Index

- `docs/Architecture-Overview.md`
- `docs/API-Integration.md`
- `docs/3D-Rendering.md`
- `docs/Controls.md`
- `docs/Class-Diagrams.md`
- `docs/Build-Instructions.md`
