#include <gtest/gtest.h>

#include "core/serialization/GameStateSerializer.h"

namespace {

/// Smoke compile: exported serialization headers parse under repo-root includes.
class SerializationHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(SerializationHeaderCompileTest, ExportedHeaderIncluded) {
    SUCCEED();
}
