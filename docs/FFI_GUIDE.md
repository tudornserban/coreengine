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
| `ce_behavior_register` | **Deprecated no-op** — does not register creators; use `ce_behavior_register_accel` or C++ `BehaviorFactory::Register` |
| `ce_behavior_register_accel` | Registers a direct-acceleration behavior from a C callback (`CE_AccelFn`); builtins still register at `ce_engine_create` |
| Built-in steering | Seek/Flee/Wander/Cohesion/Separation now run via `CalculateAcceleration` when they emit no field items, then map-edge static forces apply |
| `ce_engine_reset` | Clears units, deployment timers, and game time; **rebuilds `RTSGameState` with the same map size and RNG seed** passed to `ce_engine_create` (including seed **0**, which is deterministic) |
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
from ctypes import c_float, c_int, c_uint64

lib = ctypes.CDLL("./libcoreengine_ffi.so")
lib.ce_engine_create.restype = ctypes.c_void_p
lib.ce_engine_create.argtypes = [c_float, c_float, c_uint64]
lib.ce_engine_update.restype = c_int
lib.ce_engine_update.argtypes = [ctypes.c_void_p, c_float]
lib.ce_timer_has_charge.restype = c_int
lib.ce_timer_has_charge.argtypes = [ctypes.c_void_p, c_int]
lib.ce_unit_spawn.restype = c_int
lib.ce_unit_spawn.argtypes = [ctypes.c_void_p, c_int, c_float, c_float]

eng = lib.ce_engine_create(1600.0, 900.0, 0)
while lib.ce_timer_has_charge(eng, 1) == 0:
    lib.ce_engine_update(eng, 1.0 / 30.0)
lib.ce_unit_spawn(eng, 1, 400.0, 450.0)
```

See `ffi/python/engine.pyi` for grant, `ce_last_result`, and the rest of the surface.

## Python (cffi) — alternative

`ffi.cdef` accepts **declarations only** — drop every preprocessor line (`#…`), the `extern "C" {` / `}` wrapper lines, and `#include` / `#define` macros from `ffi/c_api.h`.

```python
from pathlib import Path
from cffi import FFI

def cdef_from_c_api_header(path: str) -> str:
    out: list[str] = ["typedef unsigned long long uint64_t;"]
    for line in Path(path).read_text().splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if s in ('extern "C" {', '}'):
            continue
        out.append(line)
    return "\n".join(out)

ffi = FFI()
ffi.cdef(cdef_from_c_api_header("ffi/c_api.h"))
lib = ffi.dlopen("./build/libcoreengine_ffi.so")

eng = lib.ce_engine_create(1600.0, 900.0, 0)
print("grant", lib.ce_timer_grant_charge(eng, 1))
print("spawn", lib.ce_unit_spawn(eng, 1, 400.0, 450.0))
print("last_result", lib.ce_last_result(eng))
lib.ce_engine_destroy(eng)
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
| `ce_engine_reset(engine)` | Reset units/state; **restores create-time RNG seed** (`ce_engine_get_seed`) |
| `ce_engine_get_seed(engine)` | Read live LCG seed (`0` if engine is null) |
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
| `ce_territory_get_pct(engine, team)` | Territory **0–100** percent (teams 1 and 2; not normalized yet) |
| `ce_behavior_list(buf, size)` | List registered behaviors |
| `ce_behavior_register(name)` | **Deprecated no-op** (see limitations) |
| `ce_behavior_register_accel(name, fn, userdata)` | Register direct-acceleration behavior (`1` / `0`) |
| `ce_version()` | Engine version string |
| `ce_ffi_version()` | FFI package version (`CE_FFI_VERSION`) |
