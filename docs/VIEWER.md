# CoreEngine SDL2 simulation viewer

Optional manual test tool under `tools/viewer/`. It is **not** part of the module registry and does not change engine APIs.

## Build (default OFF)

The viewer is gated by CMake option `COREENGINE_BUILD_SDL2_VIEWER` (default `OFF`). Default CI and local gates build and test **without** SDL2 installed.

```bash
# Ubuntu / Debian
sudo apt-get install -y libsdl2-dev

cmake -S . -B build-viewer -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_BUILD_SDL2_VIEWER=ON
cmake --build build-viewer --parallel
```

Executable: `build-viewer/tools/viewer/coreengine_sdl2_viewer`  
Viewer unit tests: `build-viewer/tools/viewer/coreengine_sdl2_viewer_tests` (or `ctest -R coreengine_sdl2_viewer_tests`).

With the flag absent, `cmake --build build --target help` must **not** list `coreengine_sdl2_viewer`.

## Command-line flags

| Flag | Meaning |
|------|---------|
| `--frames N` | Present/simulate N frames (0 = until quit in window mode) |
| `--no-window` | Headless: rasterize only, no SDL window (forces at least 1 frame when `N` is 0) |
| `--screenshot PATH` | Write the software framebuffer as PNG |
| `--script PATH` | Run script commands before the interactive/headless loop |
| `--preset NAME` | Start scene: `two-units`, `collision`, `combat`, `ai` (default `two-units`) |
| `--width W` / `--height H` | Viewport pixels (default 1280×720) |

Exit codes: `0` success, `2` bad CLI args, `3` SDL failure, `4` PNG write failure, `5` script read/parse/exec error.

## Keyboard (window mode)

| Key | Action |
|-----|--------|
| Space | Toggle pause |
| 1 / 2 / 3 / 4 | Half / normal / double / quad speed |
| `.` (period) | Single step while paused |
| R | Reset current preset (rebuild scene + engine timing) |
| F1–F6 | Toggle overlays: collisions, perception, influence, obstacles, behavior items, explosions |

## Mouse (window mode)

| Input | Action |
|-------|--------|
| Left click | Select unit (engine hit test) |
| Left drag | Box select |
| Right click | Move selected units (`CommandSystem::MoveUnitsTo`) |
| Mouse wheel | Zoom around cursor |
| Middle drag | Pan camera (see convention below) |

**Screen vs world Y:** Both use a top-left origin with **Y increasing downward** (same as `Camera::WorldToScreen`). Middle-mouse pan applies `Camera::Pan(screenDelta)`, which subtracts each screen-pixel delta from `center` in world space: drag **right** decreases `center.x`, drag **down** decreases `center.y` (the map moves with the pointer).

Screenshots use the **software raster buffer**, not SDL readback.

## Presets

All module presets use map `800×600`, seed `1`, and perception radius `80` (except `two-units`, unchanged from V1.2).

| CLI name | Exercises |
|----------|-----------|
| `two-units` | Default map + two idle teams |
| `collision` | Two units at `(380,300)` / `(420,300)` with opposing `SetVelocity`; viewer-local **Coast** influence behavior integrates motion until overlap (hub does not integrate bare velocity) |
| `combat` | Four units at fixed spawn positions with stored ±X velocities (the hub does **not** integrate velocity without a behavior). Scheduled center explosion at **t = 0.5 s**; visible motion is mostly **explosion impulse**, not convergence (**no HP damage**) |
| `ai` | Seek / flee / separation state machines. **Viewer-local:** `Scenario.cpp` adds a static attractor at **(700, 300)** because `RTSGameState::Update` always merges the four map-edge static field items into the influence path, so `CalculateAcceleration` on builtins is skipped; the attractor is what pulls units through `InfluenceField::CalculateTotalForce` |

## Script grammar

One command per line. `#` starts a comment. Commas are not allowed.

```
frames <n>
pause | resume
step
speed <0.5|1|2|4>
reset
preset <two-units|collision|combat|ai>
dest <unitIndex> <worldX> <worldY>
click <screenX> <screenY>
rclick <screenX> <screenY>
box <x0> <y0> <x1> <y1>
overlay <collisions|perception|influence|obstacles|behavior|explosions> <0|1>
```

Debug verb `dest` sets `SetDebugDestination` directly (not a player-facing command).

## Known limitations

1. **Collisions do not reduce HP.** `CombatManager` is unused; the visible combat effect is explosion impulse on `RTSUnit`.
2. **Explosion overlay echo.** `RTSGameState::Update` clears the engine `explosions_` vector at the end of each tick. The viewer copies pending engine explosions into **echoes** with **0.5 s** lifetime inside `NoteExplosions`, which runs **before** `Update`, so the orange ring can still draw after the engine list is empty.
3. **FSM automatic transitions do not tick.** The hub never calls `StateMachine::Update`; only manually `SetState` presets drive behaviors.
4. **`GameStateSerializer::DeserializeFromString` always returns false.** Presets are C++ factories only (no JSON scenario load).

## Test coverage notes

- **Key mapping** (`ViewerKeyFromKeycode`, `OverlayFKeyIndexFromKeycode`, `HandleViewerKeycode`) is covered in `coreengine_sdl2_viewer_tests` with literal SDL keycode integers (no `SDL.h` in the logic library).
- **SDL presenter path:** `SdlPresenter.cpp` forwards `SDL_KEYDOWN` `keysym.sym` values into `HandleViewerKeycode`; that wiring is not exercised without a real SDL window (headless tests call `HandleViewerKeycode` / `FrameLoop::ApplyViewerKeyAction` directly).

## Smoke commands

```bash
# Empty map (V1.1 style — two-units default, overlays off)
./build-viewer/tools/viewer/coreengine_sdl2_viewer --no-window --frames 1 \
  --screenshot /tmp/viewer-v12-units.png

# Preset PNGs (V1.6)
./build-viewer/tools/viewer/coreengine_sdl2_viewer --no-window --preset collision --frames 5 \
  --screenshot /tmp/viewer-v16-collision.png

printf 'overlay explosions 1\nframes 20\n' > /tmp/v16-combat.txt
./build-viewer/tools/viewer/coreengine_sdl2_viewer --no-window --preset combat --script /tmp/v16-combat.txt \
  --screenshot /tmp/viewer-v16-combat.png

./build-viewer/tools/viewer/coreengine_sdl2_viewer --no-window --preset ai --frames 60 \
  --screenshot /tmp/viewer-v16-ai.png

# Headless window driver (optional; needs SDL offscreen/dummy)
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software \
  ./build-viewer/tools/viewer/coreengine_sdl2_viewer --frames 2 --screenshot /tmp/viewer-window.png
```

PNG validation (stdlib):

```bash
python3 - <<'PY'
import struct
p=open("/tmp/viewer-v16-collision.png","rb").read()
assert p[:8]==b"\x89PNG\r\n\x1a\n"
w,h=struct.unpack(">II", p[16:24])
print(w, h)
PY
```

V1.2 regression pixel (team 1 blue at rounded center): `(427, 360)` → RGB `(70, 140, 230)` with overlays off.
