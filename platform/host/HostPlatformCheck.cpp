#include "platform/host/HostPlatform.h"

// Compiled into coreengine_host_check. Fails the build when CMake's detected
// target platform and the compiler's predefined macros disagree.

#if defined(COREENGINE_EXPECT_LINUX) && !defined(COREENGINE_OS_LINUX)
#error "CMake target platform is linux but the compiler is not a Linux host"
#endif
#if defined(COREENGINE_EXPECT_MACOS) && !defined(COREENGINE_OS_MACOS)
#error "CMake target platform is macos but the compiler is not an Apple macOS host"
#endif
#if defined(COREENGINE_EXPECT_WINDOWS) && !defined(COREENGINE_OS_WINDOWS)
#error "CMake target platform is windows but the compiler is not a Windows host"
#endif
#if defined(COREENGINE_EXPECT_ANDROID) && !defined(COREENGINE_OS_ANDROID)
#error "CMake target platform is android but the compiler is not an Android host"
#endif
#if defined(COREENGINE_EXPECT_EMSCRIPTEN) && !defined(COREENGINE_OS_EMSCRIPTEN)
#error "CMake target platform is emscripten but the compiler is not Emscripten"
#endif

extern "C" const char* coreengine_host_platform_name(void)
{
    return COREENGINE_OS_NAME;
}
