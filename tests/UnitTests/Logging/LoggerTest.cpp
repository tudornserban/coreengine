#include <gtest/gtest.h>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif
#include "core/logging/Logger.h"

using VenusWars::Logging::LogLevel;
using VenusWars::Logging::Logger;

namespace {

#if defined(_WIN32)
int CurrentProcessId() { return _getpid(); }
#else
int CurrentProcessId() { return getpid(); }
#endif

class LoggerStateGuard {
public:
    LoggerStateGuard() : level_(Logger::GetInstance().GetLogLevel()) {}
    ~LoggerStateGuard() {
        Logger::GetInstance().DisableFileOutput();
        Logger::GetInstance().SetLogLevel(level_);
    }

private:
    LogLevel level_;
};

class IsolatedLogFile {
public:
    IsolatedLogFile() {
        const std::string name = "coreengine_logger_" + std::to_string(CurrentProcessId()) + "_"
            + std::to_string(s_counter.fetch_add(1, std::memory_order_relaxed)) + ".log";
        path_ = std::filesystem::temp_directory_path() / name;
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    ~IsolatedLogFile() {
        Logger::GetInstance().DisableFileOutput();
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    IsolatedLogFile(const IsolatedLogFile&) = delete;
    IsolatedLogFile& operator=(const IsolatedLogFile&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
    static std::atomic<int> s_counter;
};

std::atomic<int> IsolatedLogFile::s_counter{0};

std::string ReadEntireFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

} // namespace

TEST(LoggerTest, GetInstanceIsStable) {
    LoggerStateGuard guard;
    Logger& a = Logger::GetInstance();
    Logger& b = Logger::GetInstance();
    EXPECT_EQ(&a, &b);
}

TEST(LoggerTest, SetLogLevelFiltersRuntimeMessagesToFile) {
    LoggerStateGuard guard;
    IsolatedLogFile file;
    Logger& log = Logger::GetInstance();
    log.SetLogLevel(LogLevel::WARN);

    const std::string token = file.path().filename().string();
    log.EnableFileOutput(file.path().string());
    log.Log(LogLevel::INFO, "filtered-info-%s", token.c_str());
    log.Log(LogLevel::ERROR, "kept-error-%s", token.c_str());

    const std::string contents = ReadEntireFile(file.path());
    EXPECT_EQ(contents.find("filtered-info-" + token), std::string::npos);
    EXPECT_NE(contents.find("kept-error-" + token), std::string::npos);
}

TEST(LoggerTest, IsolatedLogFilesDoNotCollideAcrossInstances) {
    LoggerStateGuard guard;
    IsolatedLogFile first;
    IsolatedLogFile second;
    ASSERT_NE(first.path(), second.path());

    Logger& log = Logger::GetInstance();
    log.SetLogLevel(LogLevel::DEBUG);

    log.EnableFileOutput(first.path().string());
    log.Log(LogLevel::INFO, "only-in-first-%s", first.path().filename().string().c_str());
    log.DisableFileOutput();

    log.EnableFileOutput(second.path().string());
    log.Log(LogLevel::INFO, "only-in-second-%s", second.path().filename().string().c_str());
    log.DisableFileOutput();

    const std::string firstContents = ReadEntireFile(first.path());
    const std::string secondContents = ReadEntireFile(second.path());
    EXPECT_NE(firstContents.find("only-in-first"), std::string::npos);
    EXPECT_EQ(firstContents.find("only-in-second"), std::string::npos);
    EXPECT_NE(secondContents.find("only-in-second"), std::string::npos);
    EXPECT_EQ(secondContents.find("only-in-first"), std::string::npos);
}