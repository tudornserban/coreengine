# tools.viewer (local boundary document)

Optional SDL2 simulation viewer. Not registered in `docs/agentic/modules/registry.json`.

## Layout

- Logic sources under `tools/viewer/` (no SDL).
- SDL presentation in `tools/viewer/sdl/` only.
- Tests in `tools/viewer/tests/`.

## Build

CMake option `COREENGINE_BUILD_SDL2_VIEWER` (default OFF). Requires SDL2 via `find_package(SDL2)`.

Viewer executable (viewer ON): `build-viewer/tools/viewer/coreengine_sdl2_viewer`

## Dependencies

The logic static library links `coreengine`. It must not include `platform/`, `ffi/`, or `SDL.h`.
