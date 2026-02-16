# MMORPG SFML C++ Client Specification

## 1. Objective
Build a standalone desktop MMORPG client in modern C++ (C++17+) that:
- Authenticates users with JWT (login/register flow)
- Connects to `ws://localhost:8080/v1/world/ws`
- Renders a basic real-time multiplayer world in a window
- Shows local and remote players as simple procedurally generated 3D placeholder models
- Supports character selection, movement input, and camera follow
- Builds with CMake and SFML

This spec prioritizes a resilient, protocol-tolerant client because server message schemas may evolve.

## 2. Technology Stack
- Language: C++17
- Windowing/Input/2D UI: SFML (window, graphics, system)
- 3D Rendering: OpenGL fixed-function pipeline via SFML OpenGL context
- HTTP Auth: cpp-httplib (header-only)
- WebSocket: websocketpp + standalone Asio
- JSON: nlohmann/json
- Build System: CMake 3.16+

## 3. High-Level Architecture
### 3.1 Core Modules
- `GameClient`
  - Main application loop
  - State machine: Auth -> CharacterSelect -> InWorld
  - Input dispatch, fixed updates, rendering orchestration
- `HttpAuthClient`
  - Register/Login via REST endpoints
  - Parse JWT from response payload
- `WebSocketClient`
  - Connect/disconnect/send over world socket
  - Background network thread
  - Thread-safe inbound message queue
  - JWT attached to handshake request
- `WorldState`
  - Local player state (id, character, position)
  - Remote players map keyed by player id
  - Timestamp of last updates
- `Renderer3D`
  - Camera/projection setup
  - Procedural shape rendering (`cube`, `sphere`, `plane`)
  - Draw world + players

### 3.2 App States
1. `AuthScreen`
   - Form fields: username, password
   - Mode toggle: Login/Register
   - Submit action
2. `CharacterSelectionScreen`
   - Select from predefined archetypes (Warrior/Mage/Rogue)
   - Confirm selection
3. `WorldScreen`
   - Real-time 3D render
   - Movement and camera follow
   - Network sync

## 4. Server Integration Contract
### 4.1 HTTP Auth Endpoints
- Base URL: `http://localhost:8080`
- Login: `POST /v1/auth/login`
- Register: `POST /v1/auth/register`
- Request body:
  ```json
  { "username": "...", "password": "..." }
  ```
- Response expected to include JWT in any of:
  - `token`
  - `accessToken`
  - `jwt`

If request fails or JWT is missing, client displays error and stays on auth screen.

### 4.2 WebSocket Endpoint
- URL: `ws://localhost:8080/v1/world/ws`
- JWT forwarding:
  - `Authorization: Bearer <jwt>` header
  - Query fallback `?token=<jwt>`

### 4.3 World Message Handling (Protocol-Tolerant)
Because server schema may vary, client accepts multiple shapes.

Inbound message patterns handled:
- World snapshot:
  - `{ "type":"world_state", "players":[...] }`
  - or any payload containing `players` array
- Player update:
  - `{ "type":"player_update", "player":{...} }`
  - or payload containing `id/playerId` + position
- Player joined/left:
  - `{ "type":"player_joined", ... }`
  - `{ "type":"player_left", "playerId":"..." }`
- Self identification:
  - `selfId` or `playerId` fields used to bind local player id

Outbound client messages:
- On world join:
  ```json
  { "type":"join_world", "character":"Warrior" }
  ```
- Movement update (throttled ~20 Hz):
  ```json
  {
    "type":"move",
    "position": { "x": 0.0, "y": 0.5, "z": 0.0 },
    "character":"Warrior"
  }
  ```

## 5. Rendering Specification
### 5.1 Scene
- Ground plane centered at origin
- Local player: cube (distinct color)
- Remote players: spheres (distinct color)
- Optional environment props: small cubes

### 5.2 Placeholder Model Generation
- Plane: procedurally generated quad from dimensions
- Cube: procedurally generated 6-face primitive
- Sphere: generated via latitude/longitude tessellation loops

### 5.3 Camera
- Third-person follow camera
- Camera offset from local player (`behind + above`)
- `lookAt` matrix targeting player position

## 6. Input and Controls
- Movement: `WASD` and arrow keys
- Auth UI:
  - Text entry for active field
  - `Tab` switch field/mode
  - `Enter` submit
- Character select:
  - Left/Right arrows cycle classes
  - `Enter` confirm
- World:
  - `Esc` disconnect to auth screen

## 7. Concurrency Model
- Main thread:
  - SFML event processing
  - Game update loop
  - Rendering
- Network thread:
  - WebSocket event loop (`websocketpp::client::run`)
- Shared data:
  - Inbound messages buffered with mutex-protected queue
  - World state modified on main thread only

## 8. Error Handling and Resilience
- Auth errors surfaced in UI status line
- WebSocket failures show connection status and permit reconnect via auth flow
- Unknown message schemas safely ignored (no crash)
- JSON parse errors caught and logged to in-app status

## 9. Build and Project Layout
Planned layout:
- `CMakeLists.txt`
- `SPEC.md`
- `src/main.cpp`
- `src/GameClient.hpp` / `src/GameClient.cpp`
- `src/Renderer3D.hpp` / `src/Renderer3D.cpp`
- `src/WebSocketClient.hpp` / `src/WebSocketClient.cpp`
- `src/HttpAuthClient.hpp` / `src/HttpAuthClient.cpp`
- `src/WorldState.hpp`
- `src/Math3D.hpp`

Dependencies via CMake:
- SFML (system install)
- OpenGL
- Threads
- FetchContent:
  - websocketpp
  - asio
  - nlohmann_json
  - cpp-httplib

## 10. Validation Plan
1. Configure/build
   - `cmake -S . -B build`
   - `cmake --build build -j`
2. Run client
   - `./build/mmorp_client`
3. Verify flows:
   - Register/login yields JWT
   - Character selection transitions to world
   - World WebSocket connects
   - Movement keys update local position + send move packets
   - Camera tracks local player
   - Remote players appear when server broadcasts state

## 11. Non-Goals
- Full asset pipeline
- Animation rigging
- Combat/inventory/chat systems
- Production-grade prediction/reconciliation

## 12. Acceptance Criteria
- App opens SFML window and renders 3D placeholder world
- User can register/login and obtain JWT
- Client connects to `ws://localhost:8080/v1/world/ws` using JWT
- Character selection is functional
- Local movement and camera follow work
- Remote players are rendered from server updates
- Project compiles with CMake and C++17+
