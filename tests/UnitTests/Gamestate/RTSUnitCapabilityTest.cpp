#include <gtest/gtest.h>

#include "core/gamestate/RTSUnit.h"

TEST(RTSUnitCapabilityTest, ImplementsInfluenceAndCombatDefaults) {
    RTSUnit unit(Vector2(4.0f, 5.0f), 2, 12.0f, 80.0f);
    EXPECT_FLOAT_EQ(unit.GetInfluenceRadius(), 12.0f);
    unit.SetInfluenceRadius(40.0f);
    EXPECT_FLOAT_EQ(unit.GetInfluenceRadius(), 40.0f);
    EXPECT_FLOAT_EQ(unit.GetAttackRange(), 0.0f);
    EXPECT_FLOAT_EQ(unit.GetBaseDPS(), 0.0f);
    EXPECT_FALSE(unit.IsDead());

    IUnitCombat* combat = &unit;
    combat->TakeDamage(100.0f);
    EXPECT_TRUE(unit.IsDead());
    EXPECT_FLOAT_EQ(unit.GetCurrentHP(), 0.0f);

    IInfluenceEmitter* emitter = &unit;
    emitter->SetInfluenceRadius(8.0f);
    EXPECT_FLOAT_EQ(emitter->GetInfluenceRadius(), 8.0f);
}
