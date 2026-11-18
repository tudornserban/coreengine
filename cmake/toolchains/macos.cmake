# osxcross toolchain for a macOS build from Linux or another non-Mac host.
#
# OSXCROSS_ROOT is the osxcross install prefix (the directory that contains
# bin/o64-clang++ or bin/oa64-clang++). On a Mac, do not pass this file:
# ./scripts/build-host-platform.sh macos
#
# COREENGINE_MACOS_ARCH: x86_64 (default) or arm64.

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
   AND NOT OSXCROSS_ROOT
   AND NOT DEFINED ENV{OSXCROSS_ROOT})
    message(FATAL_ERROR
        "cmake/toolchains/macos.cmake is the osxcross toolchain. "
        "On a Mac, configure without it (./scripts/build-host-platform.sh macos). "
        "See docs/HOST_PLATFORMS.md.")
endif()

if(NOT OSXCROSS_ROOT AND DEFINED ENV{OSXCROSS_ROOT} AND NOT "$ENV{OSXCROSS_ROOT}" STREQUAL "")
    set(OSXCROSS_ROOT "$ENV{OSXCROSS_ROOT}")
endif()

if(NOT OSXCROSS_ROOT)
    message(FATAL_ERROR
        "macOS cross builds need OSXCROSS_ROOT (or the OSXCROSS_ROOT environment variable) "
        "pointing at an osxcross installation that contains bin/o64-clang++. "
        "On a Mac, configure without cmake/toolchains/macos.cmake. "
        "See docs/HOST_PLATFORMS.md.")
endif()

if(NOT COREENGINE_MACOS_ARCH AND DEFINED ENV{COREENGINE_MACOS_ARCH}
   AND NOT "$ENV{COREENGINE_MACOS_ARCH}" STREQUAL "")
    set(COREENGINE_MACOS_ARCH "$ENV{COREENGINE_MACOS_ARCH}")
endif()
if(NOT COREENGINE_MACOS_ARCH)
    set(COREENGINE_MACOS_ARCH "x86_64")
endif()

set(CMAKE_SYSTEM_NAME Darwin)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version for CoreEngine")
endif()

if(COREENGINE_MACOS_ARCH STREQUAL "arm64" OR COREENGINE_MACOS_ARCH STREQUAL "aarch64")
    set(CMAKE_SYSTEM_PROCESSOR arm64)
    set(_ce_osx_cxx "${OSXCROSS_ROOT}/bin/oa64-clang++")
    set(_ce_osx_c "${OSXCROSS_ROOT}/bin/oa64-clang")
    if(NOT EXISTS "${_ce_osx_cxx}")
        file(GLOB _ce_osx_cxx_glob
            "${OSXCROSS_ROOT}/bin/arm64-apple-darwin*-clang++"
            "${OSXCROSS_ROOT}/bin/aarch64-apple-darwin*-clang++")
        file(GLOB _ce_osx_c_glob
            "${OSXCROSS_ROOT}/bin/arm64-apple-darwin*-clang"
            "${OSXCROSS_ROOT}/bin/aarch64-apple-darwin*-clang")
        list(SORT _ce_osx_cxx_glob)
        list(SORT _ce_osx_c_glob)
        list(LENGTH _ce_osx_cxx_glob _ce_osx_n)
        if(_ce_osx_n GREATER 0)
            list(GET _ce_osx_cxx_glob 0 _ce_osx_cxx)
            list(GET _ce_osx_c_glob 0 _ce_osx_c)
        endif()
    endif()
elseif(COREENGINE_MACOS_ARCH STREQUAL "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(_ce_osx_cxx "${OSXCROSS_ROOT}/bin/o64-clang++")
    set(_ce_osx_c "${OSXCROSS_ROOT}/bin/o64-clang")
    if(NOT EXISTS "${_ce_osx_cxx}")
        file(GLOB _ce_osx_cxx_glob "${OSXCROSS_ROOT}/bin/x86_64-apple-darwin*-clang++")
        file(GLOB _ce_osx_c_glob "${OSXCROSS_ROOT}/bin/x86_64-apple-darwin*-clang")
        list(SORT _ce_osx_cxx_glob)
        list(SORT _ce_osx_c_glob)
        list(LENGTH _ce_osx_cxx_glob _ce_osx_n)
        if(_ce_osx_n GREATER 0)
            list(GET _ce_osx_cxx_glob 0 _ce_osx_cxx)
            list(GET _ce_osx_c_glob 0 _ce_osx_c)
        endif()
    endif()
else()
    message(FATAL_ERROR
        "COREENGINE_MACOS_ARCH='${COREENGINE_MACOS_ARCH}' is not supported. Use x86_64 or arm64.")
endif()

if(NOT EXISTS "${_ce_osx_cxx}")
    message(FATAL_ERROR
        "OSXCROSS_ROOT (${OSXCROSS_ROOT}) has no clang++ for ${COREENGINE_MACOS_ARCH}. "
        "See docs/HOST_PLATFORMS.md.")
endif()

set(CMAKE_C_COMPILER "${_ce_osx_c}" CACHE FILEPATH "osxcross C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${_ce_osx_cxx}" CACHE FILEPATH "osxcross C++ compiler" FORCE)

set(CMAKE_FIND_ROOT_PATH "${OSXCROSS_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
