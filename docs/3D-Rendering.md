# 3D Rendering

## Rendering Stack

- Window/context: SFML (`sf::RenderWindow`) with OpenGL context settings
- Pipeline style: fixed-function OpenGL
- Renderer class: `Renderer3D`
- Math utilities: `Vec3` in `src/Math3D.hpp`

## OpenGL Initialization

`Renderer3D::initGL()` enables:

- Depth testing (`GL_DEPTH_TEST`)
- Back-face culling (`GL_CULL_FACE`)
- Color material (`GL_COLOR_MATERIAL`)
- Smooth shading (`GL_SMOOTH`)

`Renderer3D::resize()` updates:

- `glViewport`
- Perspective projection matrix (60 deg FOV, near 0.1, far 200)

## Camera Model

Camera is third-person follow:

- Target: `localPlayer.position + (0, 1, 0)`
- Eye offset from target: `(-5, 4, -5)`
- View built by custom `lookAt()` implementation

## Placeholder Geometry

Procedural primitives are drawn every frame:

- Ground plane: `drawPlane(60.0f)`
- Environment props: small cubes at fixed world positions
- Local player: blue cube (`drawCube(1.0f)`)
- Remote players: orange spheres (`drawSphere(0.55f, 12, 16)`)

No external mesh/asset pipeline is required for current visuals.

## Frame Render Order

1. Clear color/depth buffers
2. Set camera
3. Draw ground plane
4. Draw static props
5. Draw local player
6. Draw remote players
7. Return to `GameClient` for SFML text overlay (`pushGLStates/popGLStates`)

## Renderer Diagram

```mermaid
flowchart TD
    A[Renderer3D::render(world)] --> B[Clear Buffers]
    B --> C[setCamera(local player)]
    C --> D[drawPlane]
    D --> E[draw static cubes]
    E --> F[draw local player cube]
    F --> G[draw remote player spheres]
    G --> H[GameClient draws HUD text]
```
