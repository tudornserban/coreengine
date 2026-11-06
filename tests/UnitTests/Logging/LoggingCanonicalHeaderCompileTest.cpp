#include <gtest/gtest.h>
#include "core/logging/Logger.h"

TEST(LoggingCanonicalHeaderCompileTest, MacrosAndLevelEnumVisible) {
    EXPECT_LT(VenusWars::Logging::LogLevel::DEBUG, VenusWars::Logging::LogLevel::INFO);
    VenusWars::Logging::Logger& log = VenusWars::Logging::Logger::GetInstance();
    (void)log;
}