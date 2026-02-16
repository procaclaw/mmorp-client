# Architecture Overview

## High-Level Design

The client is organized around a single main loop (`GameClient`) that coordinates:

- UI and state transitions (`Auth -> CharacterSelect -> World`)
- HTTP authentication (`HttpAuthClient`)
- WebSocket networking (`WebSocketClient`)
- World simulation data (`WorldState`)
- 3D rendering (`Renderer3D`)

## Runtime Components

```mermaid
flowchart TD
    A[main.cpp] --> B[GameClient]
    B --> C[HttpAuthClient]
    B --> D[WebSocketClient]
    B --> E[WorldState]
    B --> F[Renderer3D]
    B --> G[SFML Window + Events]
    D --> H[(Network Thread)]
    H --> I[Inbound Message Queue]
    I --> B
```

## State Machine

```mermaid
stateDiagram-v2
    [*] --> Auth
    Auth --> CharacterSelect: Auth success (JWT)
    Auth --> Auth: Auth failure
    CharacterSelect --> World: Enter (connect WS)
    CharacterSelect --> CharacterSelect: Left/Right select class
    World --> Auth: Esc / disconnect
    World --> World: update + render + network polling
```

## Frame Lifecycle

```mermaid
flowchart LR
    A[Poll SFML Events] --> B[Update State]
    B --> C[Process WS Messages]
    C --> D[Apply Movement]
    D --> E[Send join/move packets]
    E --> F[Render UI or 3D]
    F --> G[Display frame]
```

## Data Ownership

- `WorldState` is owned by `GameClient` and mutated on the main thread.
- `WebSocketClient` owns a mutex-protected inbound queue populated on a network thread.
- `GameClient::processNetworkMessages()` drains queue data and updates `WorldState`.
- Renderer is stateless across frames except OpenGL state; it receives `const WorldState&`.
