#pragma once

// Unity renderer adapter stub.
//
// PORTING GUIDE (Unity):
// Unity uses C# for game scripting. To use the C++ CoreEngine in Unity:
// 1. Compile CoreEngine as a shared library: libcoreengine_ffi.so / .dll
// 2. Use the flat C API in ffi/c_api.h from C# via P/Invoke
// 3. Implement Unity MonoBehaviours that call ffi/ functions each frame
//
// Direct C++ integration is also possible via Unity's NativePlugin system
// but the C API path is recommended for maximum portability.
//
// See ffi/c_api.h and docs/FFI_GUIDE.md for the complete C API reference.

// This header is a placeholder for a native Unity plugin renderer.
// The actual integration lives in ffi/ and Unity C# scripts.

#ifdef __cplusplus
extern "C" {
#endif

// Unity C# P/Invoke entry points are declared in ffi/c_api.h
// Do not add additional entry points here — keep the C API surface minimal.

#ifdef __cplusplus
}
#endif
