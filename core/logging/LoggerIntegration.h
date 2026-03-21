#pragma once

#include "Logger.h"

namespace CoreEngine {
namespace Logging {

/// Optional: hook into an SDL2 log system.
/// Only available if SDL.h is included before this header.
inline void SetupSDL2Logging()
{
#if defined(SDL_VERSION_ATLEAST)
    SDL_LogSetOutputFunction([](void*, int, SDL_LogPriority priority, const char* message)
    {
        LogLevel level = LogLevel::DEBUG;
        switch (priority) {
            case SDL_LOG_PRIORITY_WARN:     level = LogLevel::WARN;  break;
            case SDL_LOG_PRIORITY_ERROR:    level = LogLevel::ERROR; break;
            case SDL_LOG_PRIORITY_CRITICAL: level = LogLevel::FATAL; break;
            default: break;
        }
        VenusWars::Logging::Logger::GetInstance().Log(level, "[SDL2] %s", message);
    }, nullptr);
#endif
}

} // namespace Logging
} // namespace CoreEngine
