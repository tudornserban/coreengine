#include <gtest/gtest.h>

#include "core/combat/CombatManager.h"
#include "core/combat/CommandSystem.h"
#include "core/combat/Explosion.h"
#include "core/combat/SelectionSystem.h"

namespace {

/// Smoke compile: exported combat headers parse under repo-root includes.
class CombatHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(CombatHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}
