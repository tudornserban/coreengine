#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include "core/logging/Logger.h"

using VenusWars::Logging::LogLevel;
using VenusWars::Logging::Logger;

TEST(LoggerTest, GetInstanceIsStable) {
    Logger& a = Logger::GetInstance();
    Logger& b = Logger::GetInstance();
    EXPECT_EQ(&a, &b);
}

TEST(LoggerTest, SetLogLevelFiltersRuntimeMessages) {
    Logger& log = Logger::GetInstance();
    const LogLevel previous = log.GetLogLevel();
    log.SetLogLevel(LogLevel::WARN);

    const std::string path =
        (std::filesystem::temp_directory_path() / "coreengine_logger_filter.log").string();
    log.EnableFileOutput(path);
    log.Log(LogLevel::INFO, "filtered-info");
    log.Log(LogLevel::ERROR, "kept-error");
    log.DisableFileOutput();

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("filtered-info"), std::string::npos);
    EXPECT_NE(contents.find("kept-error"), std::string::npos);

    log.SetLogLevel(previous);
}

TEST(LoggerTest, LogLevelToStringMatchesEnum) {
    EXPECT_STREQ(VenusWars::Logging::LogLevelToString(LogLevel::DEBUG), "DEBUG");
    EXPECT_STREQ(VenusWars::Logging::LogLevelToString(LogLevel::FATAL), "FATAL");
}
