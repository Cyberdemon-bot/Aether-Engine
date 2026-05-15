
# Aether Engine

A 3D game engine built from scratch. Designed to be lightweight and flexible for developing 3D games ranging from simple to mid-complexity.

Built as a learning project to deeply understand the internals of a modern game engine — rendering pipeline, physics, skeletal animation, audio, and scene management — without relying on commercial engines like Unity or Unreal.

---

## Demo — Zombie Shooter

A fully playable zombie shooter game built on top of Aether, used to validate all engine systems in a real game context.

| Feature | Description |
|---|---|
| Player Controller | First-person and third-person camera, smooth head bob while moving |
| Gun System | Shooting, reloading, limited ammo, muzzle flash, sound effects |
| Zombie AI | Flow Field pathfinding — optimized for up to 100 simultaneous agents |
| Dynamic Map Chunks | Map loads/unloads in chunks around the player, resource reuse |
| Obstacle Map | 16×16 static obstacle grid with walls, slow zones, and open space |
| Health & Game Over | Damage cooldown, health bar UI, respawn system |
| Radar | Real-time minimap showing zombie positions relative to the player |

---

## Features

### Rendering
Multi-pass rendering pipeline built directly on OpenGL 4.1.

- **Shadow Mapping** — real-time shadows for directional and spot lights, up to 4 simultaneous shadow casters
- **Volumetric Lighting** — god rays / light scattering with adjustable density, intensity, and ray march steps
- **PBR Shading** — Physically Based Rendering for materials
- **Skinned Mesh in Instanced Rendering** — skeletal deformation on animated meshes
- **Fog System** — linear and exponential fog modes, configurable color and density
- **LUT Color Grading** — post-process color grading via lookup texture
- **Skybox** — panoramic skybox support
- **Multi-pass Pipeline** — fully configurable render pass chain with FBO read/write

### Physics (Jolt Physics)
- **Rigid Body Simulation** — dynamic, kinematic, and static bodies
- **Collider Shapes** — box and capsule, with mesh-derived AABB for auto-fitting
- **Raycast System** — used for hitscan shooting and scene queries
- **Force & Velocity Control** — apply forces and set velocities directly on rigid bodies
- **CanMove Queries** — swept collision checks for character movement

### Animation (ozz-animation)
- **Skeletal Animation** — full bone animation from glTF/GLB files
- **Multiple Animators** — multiple simultaneous animators per scene, each entity has its own
- **Animation Controls** — play, pause, loop toggle per animator
- **Bone Attachment** — attach any entity to a named bone on a skeleton; the entity follows the bone transform automatically during animation
- **PostEvalCallback** - allow developer can activate advanced feature such as inverse kinematic, ...

### ECS & Scene (EnTT)
- **Entity-Component-System** — data-oriented architecture, cache-friendly
- **Scene Hierarchy** — parent/child entity relationships with dirty transform propagation
- **Scene Serialization** — save and load scenes to/from YAML
- **Component Queries** — `View<>` multi-component queries across the ECS

### Asset Pipeline
- **Async Model Loading** — parsing happens on a worker thread via `JobSystem`; GPU upload is deferred to the main thread safely
- **Import Cache** — fast re-import path for previously loaded assets
- **Asset Manager** — UUID-based asset tracking and lifetime management
- **glTF/GLB Support** — mesh, material, skeleton, and animation import via cgltf

### Audio (SoLoud)
- **2D Audio** — background music and UI sounds
- **3D Spatial Audio** — positional sound in world space
- **Runtime Audio Management** — create, play, loop, and volume control per source

### Scripting
- **Script Engine** — attach script instances to entities at runtime
- **Hot Reload** — load/reload a script from file path without restarting
- **Per-entity Scripts** — each entity can have an independent script instance
- **Linking** - fully funtional event manager and direct api call protocol for both script - script and engine - script sides

### Editor UI (ImGui)
- **Hierarchy Panel** — scene entity tree view
- **Scene Panel** — transform inspector, camera info, physics controls, scene save/load
- **Animation Panel** — mesh-to-animator binding, per-animator playback controls
- **Lighting Panel** — light inspector, volumetric lighting controls, shadow bias
- **Scripting Panel** — attach/detach scripts to entities at runtime
- **Bone Attachment Panel** — attach entities to skeleton bones, view active attachments
- **Performance Overlay** — real-time frame stats

---

## Architecture

Aether is built around a **Layer system** combined with **ECS (EnTT)**. Systems are organized as independent modules that communicate through an Event System and Scene Manager.

- **Layer System** — game logic is split into separate Layers (e.g. game layer, lab/editor layer), each with its own lifecycle (`Attach`, `Detach`, `Update`, `OnImGuiRender`, `OnEvent`)
- **Event System** — mouse, keyboard, and window events dispatched via Dispatcher/Listener pattern
- **Scene Graph** — hierarchical entity tree with world transform propagation
- **Render Pipeline** — configurable pass chain, each pass targets an FBO with its own shader, clear settings, and read list for post-processing

---

## Tech Stack

| | |
|---|---|
| **Language** | C++17 |
| **Graphics API** | OpenGL 4.1 |
| **Build System** | xmake |
| **Platforms** | Windows, macOS |

### Dependencies

| Library | Purpose |
|---|---|
| GLFW | Window management, input, OpenGL context |
| GLAD | OpenGL API loader |
| glm | 3D math — vectors, matrices, quaternions |
| EnTT | Entity-Component-System framework |
| ImGui | Runtime debug UI and editor panels |
| cgltf | glTF / GLB model importer |
| ozz-animation | Skeletal animation system |
| Jolt Physics | 3D physics simulation |
| SoLoud | 3D audio system |
| spdlog + fmt | Logging |
| stb | Image reading/writing (PNG, JPG) |
| yaml-cpp | Scene serializing |

---

## Building

Requires [xmake](https://xmake.io) installed.

Note: MSVC compiler is highly recommended on Windows

```bash
git clone https://github.com/Cyberdemon-bot/Aether-Engine
cd Aether-Engine
xmake
xmake run Sandbox
```

---

## Performance

Tested on real hardware running the Zombie Shooter demo:

| Device | OS | FPS |
|---|---|---|
| Windows Laptop | Windows | ~60 FPS |
| MacBook Air M4 | macOS | ~120 FPS |

---

## Known Limitations

- Obstacle map in the demo is hardcoded as a static 2D array — not yet auto-generated from scene data
- Some parameters (floor level, gun offset) are hardcoded rather than read from a config file
- No LOD system yet
- No standalone editor — editing is done through ImGui panels at runtime
- No multiplayer / networking

---