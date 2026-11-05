#include <gtest/gtest.h>
#include "core/logging/LogLevel.h"

TEST(LoggingLogLevelHeaderCompileTest, ComparisonsAndToStringFromLogLevelHeaderOnly) {
    using VenusWars::Logging::LogLevel;
    using VenusWars::Logging::LogLevelToString;
    EXPECT_LT(LogLevel::DEBUG, LogLevel::WARN);
    EXPECT_STREQ(LogLevelToString(LogLevel::FATAL), "FATAL");
}
