# Build Instructions

## Toolchain Requirements

- CMake `>= 3.16`
- C++17 compiler
- Git (required for `FetchContent` Git dependencies)

## Native Dependencies

`CMakeLists.txt` requires:

- SFML 2.5+ components: `graphics`, `window`, `system`
- OpenGL
- Threads package

And fetches:

- `asio` (`asio-1-30-2`)
- `websocketpp` (`0.8.2`)
- `nlohmann_json` (`v3.11.3`)
- `cpp-httplib` (`v0.16.3`)

## Linux (Debian/Ubuntu example)

```bash
sudo apt update
sudo apt install -y build-essential cmake git libsfml-dev libgl1-mesa-dev
```

## macOS (Homebrew example)

```bash
brew install cmake sfml
```

OpenGL framework is provided by the OS.

## Windows

- Install CMake and Visual Studio (Desktop C++ workload).
- Install SFML and expose it through `CMAKE_PREFIX_PATH` or system package manager (e.g. vcpkg).
- Ensure OpenGL development support is available (typically via Windows SDK/toolchain defaults).

## Configure and Build

From project root:

```bash
cmake -S . -B build
cmake --build build -j
```

For a specific build type:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
./build/mmorp_client
```

## Runtime Notes

- Client expects auth server on `http://localhost:8080`.
- Client expects world socket at `ws://localhost:8080/v1/world/ws`.
- If SFML cannot create a window or load fonts, ensure desktop environment and system fonts are available.
