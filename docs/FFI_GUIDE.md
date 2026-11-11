# CoreEngine — FFI Guide

How to call CoreEngine from languages other than C++.

---

## The C API Contract

The entire public surface is in `ffi/c_api.h`. Rules:

- All handles are opaque `void*` — never dereference from the foreign side
- All strings are null-terminated C strings
- All output structs (`CE_UnitState`, `CE_GameSnapshot`) are plain C POD
- No exceptions cross the boundary — most functions return `1`/`0`; string buffer calls return `-1` on error; use `ce_last_result` after spawn/timer/territory/grant calls
- Thread safety: call all functions from the same thread unless you add a mutex

---

## Deployment timer and spawning

The FFI engine owns two `DeploymentTimer` instances (10 s charge interval each). On every `ce_engine_update`, timers advance with simulated time.

| Rule | Behavior |
|------|----------|
| Fresh engine | **No** deployment charge; use `ce_timer_grant_charge(engine, team)` or ~**300** updates at `1/30` s |
| First natural charge | Arrives after **10 s** of simulated time (300 logic steps at 1/30 s) — a single `ce_engine_update(engine, 10.f)` is **not** enough (299 steps worth) |
| `ce_unit_spawn` | Returns `0` when the team has no charge or `teamId` is not 1 or 2; otherwise spends one charge and returns `1` |
| Team id | Only **`1`** and **`2`** are valid; other ids fail with `CE_ERR_INVALID_TEAM` and do not touch either timer |
| Diagnostics | `ce_last_result(engine)` returns `CE_ERR_NO_CHARGE`, `CE_ERR_INVALID_TEAM`, etc. |

Always step the simulation until `ce_timer_has_charge(engine, team)` is true, or call `ce_timer_grant_charge`, before `ce_unit_spawn`.

```c
CE_Engine eng = ce_engine_create(1600.0f, 900.0f, 42);
while (!ce_timer_has_charge(eng, 1))
    ce_engine_update(eng, 1.0f / 30.0f);
if (!ce_unit_spawn(eng, 1, 400.0f, 450.0f)) { /* still no charge */ }
```

`ce_timer_get_progress(engine, team)` returns charge fill in `[0, 1]`.

---

## Current limitations

These match today's `ffi/c_api.cpp` implementation (not aspirational docs):

| API / topic | Actual behavior |
|-------------|-----------------|
| `ce_behavior_register` | **No-op** — does not register creators; register behaviors in C++ via `BehaviorFactory::Register` before creating the engine |
| `ce_engine_reset` | Rebuilds `RTSGameState` with **rng seed 0** (drops the seed passed to `ce_engine_create`) |
| `ce_snapshot_get` | Fills map size, unit count, and elapsed time; **`winner` is always `0`**; `matchOver` is `1` only when the internal `running` flag is false (nothing in core sets a match winner today) |
| Built-in behavior names | `Seek`, `Flee`, `Wander`, `Cohesion`, `Separation` from `BehaviorFactory::RegisterBuiltins()` — **`Aggro` is not built-in** |
| `ce_unit_set_behavior` | Requires an already-registered behavior name (`BehaviorFactory::IsRegistered`) |

---

## Building the shared library

```bash
cmake -S . -B build -DBUILD_FFI=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
# Output: build/libcoreengine_ffi.dylib  (macOS)
#         build/libcoreengine_ffi.so      (Linux)
#         build/coreengine_ffi.dll        (Windows)
```

Or run `./scripts/verify-ffi-build.sh` from the repo root.

---

## Python (ctypes)

See `ffi/python/engine.pyi` for the full example.

```python
import ctypes
lib = ctypes.CDLL("./libcoreengine_ffi.so")
# ... (see engine.pyi for full setup)
eng = lib.ce_engine_create(1600.0, 900.0, 0)
while not lib.ce_timer_has_charge(eng, 1):
    lib.ce_engine_update(eng, 1.0 / 30.0)
lib.ce_unit_spawn(eng, 1, 400.0, 450.0)
```

## Python (cffi) — alternative

```python
from cffi import FFI
ffi = FFI()
with open("ffi/c_api.h") as f:
    ffi.cdef(f.read().replace("#pragma once", "").replace('#ifdef __cplusplus', '').replace('extern "C" {', '').replace('#endif', '').replace('}', '', 1))
lib = ffi.dlopen("./libcoreengine_ffi.so")
eng = lib.ce_engine_create(1600, 900, 0)
while not lib.ce_timer_has_charge(eng, 1):
    lib.ce_engine_update(eng, 1.0 / 30.0)
lib.ce_unit_spawn(eng, 1, 400, 450)
```

---

## Lua (LuaJIT FFI)

See `ffi/lua/engine.lua`:

```lua
local ce = require "engine"

local eng = ce.create(1600, 900, 0)
while not ce.has_charge(eng, 1) do
    ce.update(eng, 1/30)
end
ce.spawn(eng, 1, 400, 450)
ce.update(eng, 1/30)
print(ce.snapshot_json(eng))
ce.destroy(eng)
```

---

## Rust (bindgen)

```toml
# Cargo.toml
[build-dependencies]
bindgen = "0.69"

[dependencies]
# link against libcoreengine_ffi
```

```rust
// build.rs
bindgen::Builder::default()
    .header("ffi/c_api.h")
    .generate().unwrap()
    .write_to_file("src/bindings.rs").unwrap();
```

---

## Go (cgo)

```go
package main

// #cgo LDFLAGS: -L. -lcoreengine_ffi
// #include "ffi/c_api.h"
import "C"

func main() {
    eng := C.ce_engine_create(1600, 900, 0)
    for C.ce_timer_has_charge(eng, 1) == 0 {
        C.ce_engine_update(eng, C.float(1.0/30.0))
    }
    C.ce_unit_spawn(eng, 1, 400, 450)
    C.ce_engine_update(eng, C.float(1.0/30.0))
    C.ce_engine_destroy(eng)
}
```

---

## Unity C# (P/Invoke)

See `PORTING_GUIDE.md` Unity section and `platform/unity/UnityRenderer.h`.

---

## WebAssembly (Emscripten)

```bash
emcmake cmake -S . -B build-ffi -DBUILD_FFI=ON -G Ninja
cmake --build build-ffi
# → coreengine_ffi.wasm + JS glue from Emscripten
```

Then call from JavaScript (wait for deployment charge before spawn):

```javascript
const { ce_engine_create, ce_engine_update, ce_unit_spawn, ce_timer_has_charge } = Module;
const eng = ce_engine_create(1600, 900, 0n);
while (!ce_timer_has_charge(eng, 1)) ce_engine_update(eng, 1.0 / 30.0);
ce_unit_spawn(eng, 1, 400, 450);
```

---

## Available API Functions

| Function | Description |
|----------|-------------|
| `ce_engine_create(w, h, seed)` | Create engine instance (`DeterministicRNG`; **seed 0 is deterministic**, same LCG stream every run) |
| `ce_engine_destroy(engine)` | Free engine |
| `ce_engine_update(engine, dt)` | Tick simulation (advances deployment timers) |
| `ce_engine_reset(engine)` | Reset units/state; **re-seeds RNG to 0** |
| `ce_unit_spawn(engine, team, x, y)` | Spawn unit (requires deployment charge) |
| `ce_unit_count(engine)` | Count units |
| `ce_unit_get_state(engine, index, out)` | Read unit state |
| `ce_unit_set_behavior(engine, index, name)` | Change unit behavior (registered name only) |
| `ce_field_query(engine, x, y)` | Sample influence field |
| `ce_snapshot_get(engine, out)` | Match summary (winner not populated) |
| `ce_snapshot_get_json(engine, buf, size)` | Get JSON snapshot |
| `ce_timer_has_charge(engine, team)` | Check spawn charge (teams 1 and 2) |
| `ce_timer_get_progress(engine, team)` | Charge progress [0,1] |
| `ce_timer_grant_charge(engine, team)` | Grant one charge immediately (FFI 1.1.0+) |
| `ce_last_result(engine)` | Last spawn/timer/territory/grant result code |
| `ce_territory_get_pct(engine, team)` | Territory % (teams 1 and 2) |
| `ce_behavior_list(buf, size)` | List registered behaviors |
| `ce_behavior_register(name)` | **No-op hook** (see limitations) |
| `ce_version()` | Engine version string |
| `ce_ffi_version()` | FFI package version (`CE_FFI_VERSION`) |
