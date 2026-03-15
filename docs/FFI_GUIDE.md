# CoreEngine — FFI Guide

How to call CoreEngine from languages other than C++.

---

## The C API Contract

The entire public surface is in `ffi/c_api.h`. Rules:

- All handles are opaque `void*` — never dereference from the foreign side
- All strings are null-terminated C strings
- All output structs (`CE_UnitState`, `CE_GameSnapshot`) are plain C POD
- No exceptions cross the boundary — errors return 0 or -1
- Thread safety: call all functions from the same thread unless you add a mutex

---

## Building the shared library

```bash
cd CoreEngine
cmake -B build -DBUILD_FFI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Output: build/libcoreengine_ffi.dylib  (macOS)
#         build/libcoreengine_ffi.so      (Linux)
#         build/coreengine_ffi.dll        (Windows)
```

---

## Python (ctypes)

See `ffi/python/engine.pyi` for the full example.

```python
import ctypes
lib = ctypes.CDLL("./libcoreengine_ffi.dylib")
# ... (see engine.pyi for full setup)
eng = lib.ce_engine_create(1600.0, 900.0, 0)
```

## Python (cffi) — alternative

```python
from cffi import FFI
ffi = FFI()
with open("ffi/c_api.h") as f:
    ffi.cdef(f.read().replace("#pragma once", "").replace('#ifdef __cplusplus', '').replace('extern "C" {', '').replace('#endif', '').replace('}', '', 1))
lib = ffi.dlopen("./libcoreengine_ffi.dylib")
eng = lib.ce_engine_create(1600, 900, 0)
```

---

## Lua (LuaJIT FFI)

See `ffi/lua/engine.lua`:

```lua
local ce = require "ffi/lua/engine"
local eng = ce.create(1600, 900, 0)
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
    .header("CoreEngine/ffi/c_api.h")
    .generate().unwrap()
    .write_to_file("src/bindings.rs").unwrap();
```

---

## Go (cgo)

```go
package main

// #cgo LDFLAGS: -L. -lcoreengine_ffi
// #include "CoreEngine/ffi/c_api.h"
import "C"

func main() {
    eng := C.ce_engine_create(1600, 900, 0)
    C.ce_unit_spawn(eng, 1, 400, 450)
    C.ce_engine_update(eng, 1.0/30.0)
    C.ce_engine_destroy(eng)
}
```

---

## Unity C# (P/Invoke)

See `PORTING_GUIDE.md` Unity section and `platform/unity/UnityRenderer.h`.

---

## WebAssembly (Emscripten)

```bash
emcmake cmake -DBUILD_FFI=ON ..
emmake make
# → coreengine_ffi.wasm + coreengine_ffi.js
```

Then call from JavaScript:

```javascript
const { ce_engine_create, ce_engine_update, ce_unit_spawn } = Module;
const eng = ce_engine_create(1600, 900, 0n);
ce_unit_spawn(eng, 1, 400, 450);
```

---

## Available API Functions

| Function | Description |
|----------|-------------|
| `ce_engine_create(w, h, seed)` | Create engine instance |
| `ce_engine_destroy(engine)` | Free engine |
| `ce_engine_update(engine, dt)` | Tick simulation |
| `ce_engine_reset(engine)` | Reset to initial state |
| `ce_unit_spawn(engine, team, x, y)` | Spawn unit |
| `ce_unit_count(engine)` | Count units |
| `ce_unit_get_state(engine, index, out)` | Read unit state |
| `ce_unit_set_behavior(engine, index, name)` | Change unit behavior |
| `ce_field_query(engine, x, y)` | Sample influence field |
| `ce_snapshot_get(engine, out)` | Get match summary |
| `ce_snapshot_get_json(engine, buf, size)` | Get JSON snapshot |
| `ce_timer_has_charge(engine, team)` | Check spawn charge |
| `ce_timer_get_progress(engine, team)` | Charge progress [0,1] |
| `ce_territory_get_pct(engine, team)` | Territory % |
| `ce_behavior_list(buf, size)` | List registered behaviors |
| `ce_version()` | Engine version string |
