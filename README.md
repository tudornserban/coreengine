# CoreEngine

Platform-agnostic, language-interoperable game engine core.  
Extracted from VenusWars, DynamicGO, AscendantExpanse, and RationalInsanity.

---

## What it is

A shared C++17 library providing all the systems common across the game seeds:

| System | Description |
|--------|-------------|
| Influence Fields | APF movement AI (potential fields + repulsors/attractors) |
| State Machine + Behaviors | Generic FSM with runtime behavior swap (BehaviorFactory) |
| Collision | Broadphase SpatialGrid + circle narrowphase + impulse resolution |
| Fixed-Timestep Game Loop | 30Hz logic decoupled from render Hz |
| Deployment & Think Timers | Charge-based spawn, tactical pause |
| Territory Calculator | Voronoi grid ownership % |
| ECS Map | Entities + components (obstacles, resources) |
| Deterministic RNG | Seeded LCG for replays |
| C API (FFI) | Call from Python, Rust, Go, Lua, Unity C#, WebAssembly |

---

## Structure

```
core/        ← Zero platform deps, pure C++17
platform/    ← SDL2, Raylib, null, UE5 stub, Unity stub
ffi/         ← Flat C API + Python/Lua bindings
docs/        ← Architecture, porting guide, FFI guide
```

---

## Build

```bash
cmake -B build
cmake --build build

# With FFI shared library:
cmake -B build -DBUILD_FFI=ON
cmake --build build

# With SDL2 adapter:
cmake -B build -DBUILD_SDL2=ON
cmake --build build

# With tests:
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Optional SDL2 simulation viewer

Off by default. Requires `libsdl2-dev`. Does not change default `ctest` or module manifests.

```bash
cmake -B build-viewer -DBUILD_TESTS=ON -DCOREENGINE_BUILD_SDL2_VIEWER=ON
cmake --build build-viewer
```

See [docs/VIEWER.md](docs/VIEWER.md) for flags, keys, presets, and smoke commands.

---

## Quick Start (C++)

```cpp
#include "CoreEngine/core/gamestate/RTSGameState.h"
#include "CoreEngine/core/gamestate/RTSUnit.h"
#include "CoreEngine/core/gameloop/GameLogicEngine.h"
#include "CoreEngine/core/ai/BehaviorFactory.h"
#include "CoreEngine/core/ai/Behaviors/SeekBehavior.h"

int main()
{
    BehaviorFactory::RegisterBuiltins();

    RTSGameState gameState(Vector2(1600, 900));
    GameLogicEngine engine;
    engine.SetUpdateCallback([&](float dt) { gameState.Update(dt); });

    auto unit = std::make_unique<RTSUnit>(Vector2(400, 450), 1);
    unit->GetStateMachine().AddState("Seek", std::make_unique<SeekBehavior>(Vector2(800, 450)));
    gameState.AddUnit(std::move(unit));

    // Simulate 10 seconds
    for (int i = 0; i < 300; ++i) {
        engine.Update(1.0f / 30.0f);
    }

    return 0;
}
```

---

## Docs

- [Agentic workflow (inventory & module manifests)](docs/agentic/README.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Porting Guide](docs/PORTING_GUIDE.md) — UE5, Unity, Raylib, SDL2, Python
- [FFI Guide](docs/FFI_GUIDE.md) — Python, Rust, Go, Lua, WebAssembly
- [Coding Guidelines](docs/CODING_GUIDELINES.md)
- [Game Seeds Integration](docs/GAME_SEEDS_INTEGRATION.md)