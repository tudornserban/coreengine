# CoreEngine — macOS, Windows, and Android builds

`core/` stays free of operating-system headers. The **target platform** is chosen by CMake (or by the compiler when you compile `platform/host/HostPlatform.h` inside a game project). This is separate from the rendering adapters in `platform/sdl2`, `platform/raylib`, `platform/unity`, and `platform/unreal`.

Linux remains the desktop test host (`BUILD_TESTS=ON`, the module runners, and GitLab `build-and-test`). macOS, Windows, and Android builds produce the static engine and the C FFI shared library.

| Target | CMake profile | Shared library | Typical consumer |
|--------|---------------|----------------|------------------|
| Linux | `linux` (native) | `libcoreengine_ffi.so` | Servers, the existing CI gate |
| macOS | `macos` | `libcoreengine_ffi.dylib` | Unity `Assets/Plugins`, a native Mac game |
| Windows | `windows` | `coreengine_ffi.dll` | Unity `Assets/Plugins`, MSVC or MinGW games |
| Android | `android` | `libcoreengine_ffi.so` | Unity `Assets/Plugins/Android`, a JNI app |

`COREENGINE_TARGET_PLATFORM` in `CMakeCache.txt` is one of `linux`, `macos`, `windows`, `android`, `emscripten`, or `generic`. It is detected from the toolchain. Do not set it yourself.

The static library is `libcoreengine.a` (MinGW, macOS, Android, Linux) or `coreengine.lib` (MSVC). `coreengine_host_check` is a tiny static library that fails compilation if the compiler's OS macros disagree with `COREENGINE_TARGET_PLATFORM`.

## One command

From the repository root:

```bash
./scripts/build-host-platform.sh linux
./scripts/build-host-platform.sh macos
./scripts/build-host-platform.sh windows
./scripts/build-host-platform.sh android --abi arm64-v8a
```

CMake 3.21+ can also use the presets in `CMakePresets.json` (`linux`, `macos`, `macos-osxcross`, `windows-mingw`, `windows-msvc`, `android-arm64`, `android-armv7`, `android-x86_64`).

Each script build turns on `BUILD_FFI` and leaves `BUILD_TESTS` off. Run the GoogleTest suite on Linux.

## macOS

On a Mac, Xcode or the command-line tools are enough. The deployment target defaults to **macOS 11**.

```bash
cmake -S . -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_FFI=ON
cmake --build build-macos --parallel
```

From Linux or Windows, install [osxcross](https://github.com/tpoechtrager/osxcross) and point `OSXCROSS_ROOT` at the prefix that contains `bin/o64-clang++` (or `bin/oa64-clang++` for arm64):

```bash
export OSXCROSS_ROOT="$HOME/osxcross"
export COREENGINE_MACOS_ARCH=arm64   # optional; default is x86_64
./scripts/build-host-platform.sh macos
```

`cmake/toolchains/macos.cmake` is only the osxcross file. Passing it on a Mac, with `OSXCROSS_ROOT` unset, stops configure and tells you to use the native build.

## Windows

**Visual Studio 2022** (x64), from a developer shell:

```bash
cmake --preset windows-msvc
cmake --build build-windows-msvc --config Release --target coreengine_ffi
```

That produces `coreengine_ffi.dll`. CMake exports the `extern "C"` entry points (`ce_engine_create` and the rest) from the DLL. `NOMINMAX`, `WIN32_LEAN_AND_MEAN`, and `/utf-8` are applied for MSVC so Windows headers and the CRT do not change the engine sources.

**MinGW-w64** on Windows can configure without a toolchain file. From Linux or macOS, install the `x86_64-w64-mingw32` cross compiler (Debian/Ubuntu: `g++-mingw-w64-x86-64`) and run:

```bash
./scripts/build-host-platform.sh windows
```

`cmake/toolchains/windows-mingw.cmake` looks for `x86_64-w64-mingw32-g++` on `PATH`, or for `MINGW_ROOT/bin/x86_64-w64-mingw32-g++`.

## Android

Install Android NDK **r25 or newer** and set `ANDROID_NDK` (or `ANDROID_NDK_ROOT`) to the NDK root.

```bash
export ANDROID_NDK="$HOME/android-ndk-r26d"
./scripts/build-host-platform.sh android --abi arm64-v8a
./scripts/build-host-platform.sh android --abi armeabi-v7a
./scripts/build-host-platform.sh android --abi x86_64 --platform android-24
```

Supported ABIs are `arm64-v8a`, `armeabi-v7a`, `x86_64`, and `x86`. The default API level is **android-24**. The STL defaults to **`c++_shared`** so the `.so` can sit next to Unity's shared libc++. Override with `-DANDROID_STL=c++_static` when you want the runtime packed into `libcoreengine_ffi.so`.

`libcoreengine_ffi.so` is linked with `-Wl,-z,max-page-size=16384` so the arm64 library is aligned for 16 KB page devices.

These Android configures are rejected:

- `BUILD_TESTS=ON` (GoogleTest is not in the NDK sysroot)
- any `COREENGINE_*_STANDALONE` option
- `COREENGINE_BUILD_SDL2_VIEWER=ON`

Copy `libcoreengine_ffi.so` to `Assets/Plugins/Android/libs/<abi>/` for Unity. See [PORTING_GUIDE.md](PORTING_GUIDE.md).

## Reading the OS from game code

```cpp
#include "platform/host/HostPlatform.h"

const char* os = COREENGINE_OS_NAME;  // "macos", "windows", "android", "linux", ...
```

Do not include that header from `core/`.

## CI

GitLab builds Windows with MinGW (`windows-mingw-ffi`) and Android arm64 with NDK r26d (`android-arm64-ffi`). macOS configure-time checks run in `scripts/test_host_platforms.py`: the osxcross toolchain must fail with a clear message when `OSXCROSS_ROOT` is unset, and the header macros must select macOS when `__APPLE__` and `__MACH__` are set. A Mac or an osxcross SDK is what produces `libcoreengine_ffi.dylib`.
