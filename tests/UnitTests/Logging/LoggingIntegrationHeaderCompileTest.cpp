#include <gtest/gtest.h>
#include "core/logging/LoggerIntegration.h"

TEST(LoggingIntegrationHeaderCompileTest, IntegrationHeaderIncludesLogger) {
    VenusWars::Logging::Logger& log = VenusWars::Logging::Logger::GetInstance();
    (void)log;
}