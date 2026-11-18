# Host operating-system profile for CoreEngine builds.
#
# Safe to include from `cmake -P` (detection only). The root CMakeLists calls
# coreengine_apply_host_platform() after options are parsed and before targets.
# Named profiles: linux, macos, windows, android. Emscripten stays available for
# the existing WASM recipe. Anything else is "generic" and is not a tested host.

function(coreengine_detect_target_platform out_var)
    # Android toolchains also look like Linux to the preprocessor. Prefer ANDROID
    # before CMAKE_SYSTEM_NAME so a toolchain that sets both still maps to android.
    if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(_ce_platform android)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_ce_platform windows)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_ce_platform macos)
    elseif(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(_ce_platform emscripten)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_ce_platform linux)
    else()
        set(_ce_platform generic)
    endif()
    set(${out_var} "${_ce_platform}" PARENT_SCOPE)
endfunction()

function(coreengine_apply_host_platform)
    coreengine_detect_target_platform(_ce_platform)
    set(COREENGINE_TARGET_PLATFORM "${_ce_platform}" CACHE STRING
        "Detected CoreEngine target platform (linux, macos, windows, android, emscripten, or generic)"
        FORCE)
    message(STATUS
        "CoreEngine target platform: ${COREENGINE_TARGET_PLATFORM} (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")

    if(COREENGINE_TARGET_PLATFORM STREQUAL "generic")
        message(STATUS
            "CoreEngine has named build profiles for linux, macos, windows, and android. "
            "CMAKE_SYSTEM_NAME='${CMAKE_SYSTEM_NAME}' uses the generic profile.")
    endif()

    if(COREENGINE_TARGET_PLATFORM STREQUAL "android")
        # Android loads CoreEngine from a shared object (Unity, JNI). Position-independent
        # code is required even when BUILD_FFI is OFF and only the static archive is produced.
        set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
        if(BUILD_TESTS)
            message(FATAL_ERROR
                "BUILD_TESTS is not supported when the target platform is android. "
                "GoogleTest is not part of the NDK sysroot. Reconfigure with -DBUILD_TESTS=OFF "
                "and run the test suite on a desktop host.")
        endif()
        if(_standalone_modes GREATER 0)
            message(FATAL_ERROR
                "COREENGINE_*_STANDALONE options are desktop test builds and are not supported "
                "when the target platform is android.")
        endif()
        if(COREENGINE_BUILD_SDL2_VIEWER)
            message(FATAL_ERROR
                "COREENGINE_BUILD_SDL2_VIEWER is not supported when the target platform is android.")
        endif()
    endif()

    if(COREENGINE_TARGET_PLATFORM STREQUAL "windows")
        # MSVC does not export C API symbols from a SHARED library unless they are
        # listed in a module-definition file. CMake generates that file from the
        # objects compiled into coreengine_ffi (the extern "C" entry points).
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON PARENT_SCOPE)
        add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
        if(MSVC)
            add_compile_options(/utf-8)
            add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        endif()
    endif()
endfunction()
