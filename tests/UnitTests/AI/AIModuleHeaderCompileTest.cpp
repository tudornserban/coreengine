#include <gtest/gtest.h>

#include "core/ai/BehaviorContext.h"
#include "core/ai/BehaviorFactory.h"
#include "core/ai/Behaviors/CohesionBehavior.h"
#include "core/ai/Behaviors/FleeBehavior.h"
#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/ai/Behaviors/SeparationBehavior.h"
#include "core/ai/Behaviors/WanderBehavior.h"
#include "core/ai/Conditions/AlliesNearbyCondition.h"
#include "core/ai/Conditions/EnemyInRangeCondition.h"
#include "core/ai/Conditions/TimeElapsedCondition.h"
#include "core/ai/StateMachine.h"

namespace {

/// Smoke compile: exported AI headers parse under repo-root includes.
class AIHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(AIHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}
