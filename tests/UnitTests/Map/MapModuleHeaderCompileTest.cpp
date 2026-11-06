#include <gtest/gtest.h>

#include "core/map/GameMode.h"
#include "core/map/Map.h"
#include "core/map/MapEntity.h"
#include "core/map/MapComponents/MapObstacleComponent.h"
#include "core/map/MapComponents/MapResourceComponent.h"
#include "core/map/PlacementValidator.h"

namespace {

/// Smoke compile: exported map headers parse under repo-root includes.
class MapHeaderCompileTest : public ::testing::Test {};

}  // namespace

TEST_F(MapHeaderCompileTest, AllExportedHeadersIncluded) {
    SUCCEED();
}