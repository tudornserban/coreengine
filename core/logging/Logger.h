#pragma once

#include "LogLevel.h"
#include <cstdio>
#include <string>
#include <fstream>
#include <memory>

namespace VenusWars
{
    namespace Logging
    {
        class Logger
        {
        public:
            static Logger& GetInstance();

            void SetLogLevel(LogLevel level);
            LogLevel GetLogLevel() const { return m_currentLevel; }

            void EnableFileOutput(const std::string& filename);
            void DisableFileOutput();
            bool IsFileOutputEnabled() const { return m_fileOutputEnabled; }

            void Log(LogLevel level, const char* format, ...);

            // Shutdown logger (closes file if open)
            void Shutdown();

        private:
            Logger();
            ~Logger();
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            void WriteToConsole(LogLevel level, const char* message);
            void WriteToFile(LogLevel level, const char* message);
            std::string FormatTimestamp() const;
            std::string FormatMessage(LogLevel level, const char* message) const;

            LogLevel m_currentLevel;
            bool m_fileOutputEnabled;
            std::unique_ptr<std::ofstream> m_logFile;
        };

        // Macros for compile-time filtering
        // Define LOG_MIN_LEVEL to disable levels at compile time (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL)
        #ifndef LOG_MIN_LEVEL
        #define LOG_MIN_LEVEL 0  // DEBUG by default
        #endif

        // Logging macros with compile-time filtering
        #if LOG_MIN_LEVEL <= 0
        #define LOG_DEBUG(...) VenusWars::Logging::Logger::GetInstance().Log(VenusWars::Logging::LogLevel::DEBUG, __VA_ARGS__)
        #else
        #define LOG_DEBUG(...) ((void)0)
        #endif

        #if LOG_MIN_LEVEL <= 1
        #define LOG_INFO(...) VenusWars::Logging::Logger::GetInstance().Log(VenusWars::Logging::LogLevel::INFO, __VA_ARGS__)
        #else
        #define LOG_INFO(...) ((void)0)
        #endif

        #if LOG_MIN_LEVEL <= 2
        #define LOG_WARN(...) VenusWars::Logging::Logger::GetInstance().Log(VenusWars::Logging::LogLevel::WARN, __VA_ARGS__)
        #else
        #define LOG_WARN(...) ((void)0)
        #endif

        #if LOG_MIN_LEVEL <= 3
        #define LOG_ERROR(...) VenusWars::Logging::Logger::GetInstance().Log(VenusWars::Logging::LogLevel::ERROR, __VA_ARGS__)
        #else
        #define LOG_ERROR(...) ((void)0)
        #endif

        #if LOG_MIN_LEVEL <= 4
        #define LOG_FATAL(...) VenusWars::Logging::Logger::GetInstance().Log(VenusWars::Logging::LogLevel::FATAL, __VA_ARGS__)
        #else
        #define LOG_FATAL(...) ((void)0)
        #endif
    }
}

