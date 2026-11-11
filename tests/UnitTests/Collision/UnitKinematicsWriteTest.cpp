#include <gtest/gtest.h>

#include "core/collision/CollisionResolver.h"
#include "core/collision/MapCollisionHandler.h"
#include "core/collision/PositionCorrector.h"
#include "tests/TestHelpers/KinematicMockGameUnit.h"
#include "core/math/Vector2.h"
#include "tests/TestHelpers/MockGameUnit.h"

TEST(UnitKinematicsWriteTest, ApplyImpulse_updates_kinematic_unit_via_iunit_kinematics) {
    KinematicMockGameUnit unit(Vector2::Zero(), 0, 10.0f, 100.0f);
    ASSERT_FLOAT_EQ(unit.CalculateMass(), 100.0f);
    unit.SetVelocity(Vector2(1.0f, 0.0f));

    CollisionResolver::ApplyImpulse(&unit, Vector2(10.0f, 0.0f), 100.0f);

    Vector2 v = unit.GetVelocity();
    EXPECT_FLOAT_EQ(v.x, 1.1f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(UnitKinematicsWriteTest, ApplyImpulse_skips_mock_without_iunit_kinematics) {
    MockGameUnit mock;
    mock.SetVelocity(Vector2(1.0f, 0.0f));

    CollisionResolver::ApplyImpulse(&mock, Vector2(10.0f, 0.0f), 100.0f);

    Vector2 v = mock.GetVelocity();
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(UnitKinematicsWriteTest, ApplyCorrection_moves_kinematic_unit) {
    KinematicMockGameUnit unit(Vector2::Zero(), 0);
    unit.SetPosition(Vector2(0.0f, 0.0f));

    PositionCorrector::ApplyCorrection(&unit, Vector2(3.0f, 4.0f));

    Vector2 p = unit.GetPosition();
    EXPECT_FLOAT_EQ(p.x, 3.0f);
    EXPECT_FLOAT_EQ(p.y, 4.0f);
}

TEST(UnitKinematicsWriteTest, ApplyCorrection_leaves_mock_unchanged) {
    MockGameUnit mock(Vector2(0.0f, 0.0f));

    PositionCorrector::ApplyCorrection(&mock, Vector2(3.0f, 4.0f));

    Vector2 p = mock.GetPosition();
    EXPECT_FLOAT_EQ(p.x, 0.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}

TEST(UnitKinematicsWriteTest, PushInsideBounds_clamps_kinematic_unit) {
    KinematicMockGameUnit unit(Vector2(0.0f, 0.0f), 0, 10.0f, 100.0f);

    MapCollisionHandler::PushInsideBounds(&unit, Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f));

    Vector2 p = unit.GetPosition();
    EXPECT_FLOAT_EQ(p.x, 10.0f);
    EXPECT_FLOAT_EQ(p.y, 10.0f);
}

TEST(UnitKinematicsWriteTest, PushInsideBounds_skips_mock) {
    MockGameUnit mock(Vector2(0.0f, 0.0f), 0, 10.0f, 100.0f);

    MapCollisionHandler::PushInsideBounds(&mock, Vector2(0.0f, 0.0f), Vector2(100.0f, 100.0f));

    Vector2 p = mock.GetPosition();
    EXPECT_FLOAT_EQ(p.x, 0.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}
