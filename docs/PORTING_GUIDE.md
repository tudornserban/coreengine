# CoreEngine — Porting Guide

How to use CoreEngine in different game engines and platforms.

---

## C FFI — deployment and teams

When using `ffi/c_api.h` (`coreengine_ffi`), playable teams are **`1`** and **`2`** only. A new engine has **no** spawn charge until you either call `ce_timer_grant_charge(engine, team)` or advance ~**300** logic steps at `1/30` s via `ce_engine_update`. If `ce_unit_spawn` returns `0`, call `ce_last_result(engine)` for `CE_ERR_NO_CHARGE` or `CE_ERR_INVALID_TEAM`. See [FFI_GUIDE.md](FFI_GUIDE.md) for full API notes.

---

## Unreal Engine 5

### Step 1: Copy `core/` into your UE5 project

```
cp -r core/ MyGame/Source/MyGame/CoreEngine/
```

All files in `core/` compile with `-std=c++17` and zero UE5 dependencies.

### Step 2: Add to Build.cs

```csharp
// MyGame.Build.cs
PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreEngine"));
```

### Step 3: Wrap RTSUnit in a UCLASS Actor

```cpp
UCLASS()
class MYGAME_API AMyUnit : public AActor
{
    GENERATED_BODY()
private:
    std::unique_ptr<RTSUnit> coreUnit_;

public:
    virtual void Tick(float DeltaTime) override
    {
        // Build context from UE world queries
        BehaviorContext ctx;
        ctx.self = coreUnit_.get();
        ctx.deltaTime = DeltaTime;
        ctx.rng = &gameState_->GetRNG();
        // ... populate nearbyAllies, nearbyEnemies from UE overlap sphere

        // Run AI (hub does not call StateMachine::Update — drive transitions yourself or call it here)
        Vector2 accel = coreUnit_->GetStateMachine().Update(ctx);
        coreUnit_->ApplyAcceleration(accel, DeltaTime);

        // Sync UE position
        Vector2 pos = coreUnit_->GetPosition();
        SetActorLocation(FVector(pos.x, pos.y, 0.0f));
    }
};
```

### Step 4: Use NullRenderer for logic (UE5 handles rendering)

```cpp
// Game mode or subsystem
RTSGameState gameState(Vector2(mapWidth, mapHeight), rngSeed);
GameLogicEngine logicEngine;
logicEngine.SetUpdateCallback([&](float dt) { gameState.Update(dt); });

// In Tick():
logicEngine.Update(GetWorld()->GetDeltaSeconds());
```

**All AI, influence fields, collision, and state machines work unchanged** (add static field items or `GenerateFieldItems` if you rely on APF motion — see README).

---

## Unity (via C API)

### Step 1: Build the FFI shared library

```bash
cmake -S . -B build -DBUILD_FFI=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
# → libcoreengine_ffi.dylib / .so / .dll
```

### Step 2: Copy to Unity Plugins folder

```
Assets/Plugins/libcoreengine_ffi.dylib  (macOS)
Assets/Plugins/coreengine_ffi.dll       (Windows)
Assets/Plugins/libcoreengine_ffi.so     (Linux/Android)
```

### Step 3: C# P/Invoke bindings

```csharp
using System.Runtime.InteropServices;

public static class CoreEngine
{
    const string LIB = "coreengine_ffi";

    [DllImport(LIB)] public static extern System.IntPtr ce_engine_create(float w, float h, ulong seed);
    [DllImport(LIB)] public static extern void   ce_engine_destroy(System.IntPtr engine);
    [DllImport(LIB)] public static extern int    ce_engine_update(System.IntPtr engine, float dt);
    [DllImport(LIB)] public static extern int    ce_timer_has_charge(System.IntPtr engine, int team);
    [DllImport(LIB)] public static extern int    ce_timer_grant_charge(System.IntPtr engine, int team);
    [DllImport(LIB)] public static extern int    ce_last_result(System.IntPtr engine);
    [DllImport(LIB)] public static extern int    ce_unit_spawn(System.IntPtr engine, int team, float x, float y);
    [DllImport(LIB)] public static extern int    ce_unit_count(System.IntPtr engine);
    [DllImport(LIB)] public static extern float  ce_territory_get_pct(System.IntPtr engine, int team);
    [DllImport(LIB)] public static extern int    ce_snapshot_get_json(System.IntPtr engine, byte[] buf, int size);
}

// Usage in MonoBehaviour:
void Start() {
    engine = CoreEngine.ce_engine_create(1600, 900, 0);
    while (CoreEngine.ce_timer_has_charge(engine, 1) == 0)
        CoreEngine.ce_engine_update(engine, 1f/30f);
    CoreEngine.ce_unit_spawn(engine, 1, 400, 450);
}
void Update() { CoreEngine.ce_engine_update(engine, Time.deltaTime); }
void OnDestroy() { CoreEngine.ce_engine_destroy(engine); }
```

---

## Raylib (DynamicGO pattern)

`platform/raylib/` provides **header sketches** only: `RaylibRenderer::Render()` is declared but **not implemented** in this repository (no `platform/raylib/*.cpp`). Treat the sample below as a wiring diagram — copy VenusWars/DynamicGO render code into your game target or implement `Render()` locally.

```cpp
#include "core/gamestate/RTSGameState.h"
#include "core/gameloop/GameLogicEngine.h"
#include "platform/raylib/RaylibInputReader.h"
#include "platform/raylib/RaylibRenderer.h"
#include <raylib.h>

int main()
{
    InitWindow(1600, 900, "My Game");
    SetTargetFPS(60);

    RTSGameState gameState(Vector2(1600, 900));
    GameLogicEngine logicEngine;
    logicEngine.SetUpdateCallback([&](float dt) { gameState.Update(dt); });

    RaylibRenderer renderer(1600, 900);
    renderer.SetGameState(&gameState);

    RaylibInputReader input;

    while (!WindowShouldClose()) {
        input.PollEvents();
        logicEngine.Update(GetFrameTime());
        BeginDrawing();
        renderer.Render();  // you must supply the .cpp implementation
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

---

## SDL2

For a runnable SDL2 window that ticks `RTSGameState` through `GameLogicEngine` without modifying `platform/sdl2` stubs, use the optional viewer in this repository:

- Enable `-DCOREENGINE_BUILD_SDL2_VIEWER=ON` (default OFF), install `libsdl2-dev`, build `coreengine_sdl2_viewer`.
- Full flag/key/preset/script documentation: [VIEWER.md](VIEWER.md).

`platform/sdl2` ships **header-only stubs** (`SDLRenderer::Render()` is declared, not implemented here). Game-specific ports copy VenusWars SDL code or use the viewer. `-DBUILD_SDL2=ON` **fails configure** until you add at least one `platform/sdl2/*.cpp` (empty glob → `No SOURCES given to target: coreengine_sdl2`).

---

## Python

```python
import ctypes, json

lib = ctypes.CDLL("./libcoreengine_ffi.so")
lib.ce_engine_create.restype  = ctypes.c_void_p
lib.ce_engine_create.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_uint64]
lib.ce_engine_update.restype  = ctypes.c_int
lib.ce_engine_update.argtypes = [ctypes.c_void_p, ctypes.c_float]
lib.ce_timer_has_charge.restype  = ctypes.c_int
lib.ce_timer_has_charge.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.ce_unit_spawn.restype  = ctypes.c_int
lib.ce_unit_spawn.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float, ctypes.c_float]
lib.ce_snapshot_get_json.restype  = ctypes.c_int
lib.ce_snapshot_get_json.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]

engine = lib.ce_engine_create(1600.0, 900.0, 0)
while not lib.ce_timer_has_charge(engine, 1):
    lib.ce_engine_update(engine, 1.0/30.0)
lib.ce_unit_spawn(engine, 1, 400.0, 450.0)
for _ in range(300):
    lib.ce_engine_update(engine, 1.0/30.0)

buf = ctypes.create_string_buffer(65536)
lib.ce_snapshot_get_json(engine, buf, 65536)
print(json.loads(buf.value))
lib.ce_engine_destroy(engine)
```

---

## Modification Zones

| Zone | Systems | Rule |
|------|---------|------|
| Ultra-high | `InfluenceField::CalculateTotalForce`, `InfluenceFieldItem` | Always ask |
| High | `StateMachine`, `CollisionSystem`, `RTSGameState`, `RTSUnit` | Always ask |
| Medium | Behaviors, Conditions, GameStates | Ask if modifying |
| Low | New behaviors, platform/ adapters, ffi/ bindings | Proceed |
