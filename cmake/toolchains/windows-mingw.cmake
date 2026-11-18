# MinGW-w64 cross toolchain for the Windows build (x86_64).
#
# On a Windows machine, configure with MSVC or a native MinGW environment and do not
# pass this file. See docs/HOST_PLATFORMS.md.

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND NOT MINGW_ROOT AND NOT DEFINED ENV{MINGW_ROOT})
    message(FATAL_ERROR
        "cmake/toolchains/windows-mingw.cmake is the MinGW-w64 cross toolchain. "
        "On Windows, configure with MSVC or a native MinGW shell and do not pass this file. "
        "See docs/HOST_PLATFORMS.md.")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT MINGW_ROOT AND DEFINED ENV{MINGW_ROOT} AND NOT "$ENV{MINGW_ROOT}" STREQUAL "")
    set(MINGW_ROOT "$ENV{MINGW_ROOT}")
endif()

if(MINGW_ROOT)
    set(_ce_mingw_c "${MINGW_ROOT}/bin/x86_64-w64-mingw32-gcc")
    set(_ce_mingw_cxx "${MINGW_ROOT}/bin/x86_64-w64-mingw32-g++")
    set(_ce_mingw_rc "${MINGW_ROOT}/bin/x86_64-w64-mingw32-windres")
    if(NOT EXISTS "${_ce_mingw_cxx}")
        message(FATAL_ERROR
            "MINGW_ROOT (${MINGW_ROOT}) does not contain bin/x86_64-w64-mingw32-g++. "
            "See docs/HOST_PLATFORMS.md.")
    endif()
else()
    find_program(_ce_mingw_c NAMES x86_64-w64-mingw32-gcc)
    find_program(_ce_mingw_cxx NAMES x86_64-w64-mingw32-g++)
    find_program(_ce_mingw_rc NAMES x86_64-w64-mingw32-windres)
endif()

if(NOT _ce_mingw_c OR NOT _ce_mingw_cxx)
    message(FATAL_ERROR
        "Windows cross builds need MinGW-w64 (x86_64-w64-mingw32-g++ on PATH) or MINGW_ROOT. "
        "On Windows, configure with MSVC or MinGW without cmake/toolchains/windows-mingw.cmake. "
        "See docs/HOST_PLATFORMS.md.")
endif()

set(CMAKE_C_COMPILER "${_ce_mingw_c}" CACHE FILEPATH "MinGW C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${_ce_mingw_cxx}" CACHE FILEPATH "MinGW C++ compiler" FORCE)
if(_ce_mingw_rc)
    set(CMAKE_RC_COMPILER "${_ce_mingw_rc}" CACHE FILEPATH "MinGW resource compiler" FORCE)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
