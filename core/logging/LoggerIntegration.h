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
        VenusWars::Logging::LogLevel level = VenusWars::Logging::LogLevel::DEBUG;
        switch (priority) {
            case SDL_LOG_PRIORITY_WARN:     level = VenusWars::Logging::LogLevel::WARN;  break;
            case SDL_LOG_PRIORITY_ERROR:    level = VenusWars::Logging::LogLevel::ERROR; break;
            case SDL_LOG_PRIORITY_CRITICAL: level = VenusWars::Logging::LogLevel::FATAL; break;
            default: break;
        }
        VenusWars::Logging::Logger::GetInstance().Log(level, "[SDL2] %s", message);
    }, nullptr);
#endif
}

} // namespace Logging
} // namespace CoreEngine
