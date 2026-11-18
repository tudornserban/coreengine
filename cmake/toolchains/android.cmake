# Android NDK toolchain wrapper.
#
# Usage:
#   cmake -S . -B build-android \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/android.cmake \
#     -DANDROID_NDK=$ANDROID_NDK \
#     -DANDROID_ABI=arm64-v8a
#
# ANDROID_NDK, ANDROID_NDK_ROOT, or ANDROID_NDK_HOME may also come from the environment.
# Requires NDK r25 or newer (ships build/cmake/android.toolchain.cmake).

if(NOT ANDROID_NDK AND DEFINED ENV{ANDROID_NDK} AND NOT "$ENV{ANDROID_NDK}" STREQUAL "")
    set(ANDROID_NDK "$ENV{ANDROID_NDK}")
endif()
if(NOT ANDROID_NDK AND DEFINED ENV{ANDROID_NDK_ROOT} AND NOT "$ENV{ANDROID_NDK_ROOT}" STREQUAL "")
    set(ANDROID_NDK "$ENV{ANDROID_NDK_ROOT}")
endif()
if(NOT ANDROID_NDK AND DEFINED ENV{ANDROID_NDK_HOME} AND NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
    set(ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
endif()

if(NOT ANDROID_NDK)
    message(FATAL_ERROR
        "Android builds need ANDROID_NDK or ANDROID_NDK_ROOT (CMake variable or environment) "
        "pointing at Android NDK r25 or newer. See docs/HOST_PLATFORMS.md.")
endif()

if(NOT EXISTS "${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
    message(FATAL_ERROR
        "ANDROID_NDK (${ANDROID_NDK}) does not contain build/cmake/android.toolchain.cmake. "
        "Point ANDROID_NDK at the NDK root (NDK r25 or newer).")
endif()

if(NOT ANDROID_ABI)
    set(ANDROID_ABI "arm64-v8a" CACHE STRING
        "Android ABI (arm64-v8a, armeabi-v7a, x86_64, x86)")
endif()

set(_ce_android_abis arm64-v8a armeabi-v7a x86_64 x86)
if(NOT ANDROID_ABI IN_LIST _ce_android_abis)
    message(FATAL_ERROR
        "ANDROID_ABI='${ANDROID_ABI}' is not supported. Use one of: arm64-v8a, armeabi-v7a, x86_64, x86.")
endif()

if(NOT ANDROID_PLATFORM)
    set(ANDROID_PLATFORM "android-24" CACHE STRING "Minimum Android API level")
endif()

if(NOT ANDROID_STL)
    # Shared libc++ matches Unity and other engines that already load c++_shared.
    # Override with -DANDROID_STL=c++_static for a self-contained .so.
    set(ANDROID_STL "c++_shared" CACHE STRING "Android STL (c++_shared or c++_static)")
endif()

include("${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
