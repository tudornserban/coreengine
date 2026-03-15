#include "Logger.h"
#include <ctime>
#include <cstdarg>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace VenusWars
{
    namespace Logging
    {
        Logger::Logger()
            : m_currentLevel(LogLevel::DEBUG)
            , m_fileOutputEnabled(false)
        {
        }

        Logger::~Logger()
        {
            Shutdown();
        }

        Logger& Logger::GetInstance()
        {
            static Logger instance;
            return instance;
        }

        void Logger::SetLogLevel(LogLevel level)
        {
            m_currentLevel = level;
        }

        void Logger::EnableFileOutput(const std::string& filename)
        {
            m_logFile = std::make_unique<std::ofstream>(filename, std::ios::app);
            if (m_logFile->is_open())
            {
                m_fileOutputEnabled = true;
            }
            else
            {
                m_fileOutputEnabled = false;
                m_logFile.reset();
            }
        }

        void Logger::DisableFileOutput()
        {
            if (m_logFile && m_logFile->is_open())
            {
                m_logFile->close();
            }
            m_logFile.reset();
            m_fileOutputEnabled = false;
        }

        void Logger::Log(LogLevel level, const char* format, ...)
        {
            // Runtime level filtering
            if (level < m_currentLevel)
            {
                return;
            }

            // Format the message
            char buffer[4096];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            // Write to console
            WriteToConsole(level, buffer);

            // Write to file if enabled
            if (m_fileOutputEnabled && m_logFile && m_logFile->is_open())
            {
                WriteToFile(level, buffer);
            }
        }

        void Logger::WriteToConsole(LogLevel level, const char* message)
        {
            std::string formatted = FormatMessage(level, message);
            
            // Use stderr for ERROR and FATAL, stdout for others
            if (level >= LogLevel::ERROR)
            {
                std::cerr << formatted << std::endl;
            }
            else
            {
                std::cout << formatted << std::endl;
            }
        }

        void Logger::WriteToFile(LogLevel level, const char* message)
        {
            if (m_logFile && m_logFile->is_open())
            {
                std::string formatted = FormatMessage(level, message);
                *m_logFile << formatted << std::endl;
                m_logFile->flush();  // Ensure immediate write
            }
        }

        std::string Logger::FormatTimestamp() const
        {
            std::time_t now = std::time(nullptr);
            std::tm* timeinfo = std::localtime(&now);
            
            std::ostringstream oss;
            oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        std::string Logger::FormatMessage(LogLevel level, const char* message) const
        {
            std::ostringstream oss;
            oss << "[" << FormatTimestamp() << "] "
                << "[" << LogLevelToString(level) << "] "
                << message;
            return oss.str();
        }

        void Logger::Shutdown()
        {
            DisableFileOutput();
        }
    }
}

