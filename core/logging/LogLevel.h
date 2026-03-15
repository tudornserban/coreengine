#pragma once

namespace VenusWars
{
    namespace Logging
    {
        enum class LogLevel
        {
            DEBUG = 0,
            INFO = 1,
            WARN = 2,
            ERROR = 3,
            FATAL = 4
        };

        inline bool operator<=(LogLevel lhs, LogLevel rhs)
        {
            return static_cast<int>(lhs) <= static_cast<int>(rhs);
        }

        inline bool operator>=(LogLevel lhs, LogLevel rhs)
        {
            return static_cast<int>(lhs) >= static_cast<int>(rhs);
        }

        inline bool operator<(LogLevel lhs, LogLevel rhs)
        {
            return static_cast<int>(lhs) < static_cast<int>(rhs);
        }

        inline bool operator>(LogLevel lhs, LogLevel rhs)
        {
            return static_cast<int>(lhs) > static_cast<int>(rhs);
        }

        inline const char* LogLevelToString(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARN: return "WARN";
                case LogLevel::ERROR: return "ERROR";
                case LogLevel::FATAL: return "FATAL";
                default: return "UNKNOWN";
            }
        }
    }
}

