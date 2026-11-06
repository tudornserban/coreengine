#include <gtest/gtest.h>

#include "core/collision/CollisionDetector.h"
#include "core/collision/CollisionResolver.h"
#include "core/collision/CollisionSystem.h"
#include "core/collision/ICollidable.h"
#include "core/collision/ICollisionDetector.h"
#include "core/collision/ICollisionObserver.h"
#include "core/collision/ImmunityManager.h"
#include "core/collision/PositionCorrector.h"
#include "core/collision/SpatialGrid.h"

namespace {

/// Smoke compile: exported collision headers parse under repo-root includes.
class CollisionHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(CollisionHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}
