#pragma once

// Compile-time name of the operating system this binary is built for.
// core/ must not include this header. CMake's COREENGINE_TARGET_PLATFORM and
// these macros are checked against each other by platform/host/HostPlatformCheck.cpp.
//
// Android is tested first: the NDK also defines __linux__.
// Windows is tested before Apple and Linux. Emscripten is tested before Linux.

#if defined(__ANDROID__)
#define COREENGINE_OS_ANDROID 1
#define COREENGINE_OS_NAME "android"
#elif defined(_WIN32)
#define COREENGINE_OS_WINDOWS 1
#define COREENGINE_OS_NAME "windows"
#elif defined(__EMSCRIPTEN__)
#define COREENGINE_OS_EMSCRIPTEN 1
#define COREENGINE_OS_NAME "emscripten"
#elif defined(__APPLE__) && defined(__MACH__)
#define COREENGINE_OS_MACOS 1
#define COREENGINE_OS_NAME "macos"
#elif defined(__linux__)
#define COREENGINE_OS_LINUX 1
#define COREENGINE_OS_NAME "linux"
#else
#define COREENGINE_OS_NAME "unknown"
#endif
