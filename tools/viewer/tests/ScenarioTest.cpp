#include "FrameLoop.h"
#include "Overlay.h"
#include "Scenario.h"

#include "core/collision/ICollisionObserver.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IUnitBehaviorHost.h"

#include <cmath>
#include <gtest/gtest.h>

TEST(ScenarioTest, CollisionPresetOverlapsAndRecordsAContact)
{
    Scene scene = BuildPreset(Preset::Collision, 1);
    const auto& units = scene.state->GetAllUnits();
    ASSERT_EQ(units.size(), 2u);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().x, 380.0f);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().y, 300.0f);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().x, 420.0f);
    EXPECT_EQ(units[0]->GetTeam(), 1);
    EXPECT_EQ(units[1]->GetTeam(), 2);
    EXPECT_FLOAT_EQ(units[0]->GetRadius(), 16.0f);

    CollisionRecorder recorder;
    scene.state->GetCollisionSystem().Subscribe(&recorder);
    recorder.SetReferenceGameTime(0.0f);
    for (int i = 0; i < 30 && recorder.Events().empty(); ++i)
    {
        scene.state->Update(kFixedDt);
    }
    EXPECT_GE(recorder.Events().size(), 1u);
    scene.state->GetCollisionSystem().Unsubscribe(&recorder);
}

TEST(ScenarioTest, AiSeekUnitMovesTowardTheTarget)
{
    Scene scene = BuildPreset(Preset::Ai, 1);
    ASSERT_EQ(scene.state->GetAllUnits().size(), 4u);
    const float startX = scene.state->GetAllUnits()[0]->GetPosition().x;
    EXPECT_FLOAT_EQ(startX, 120.0f);
    EXPECT_EQ(scene.state->GetAllUnits()[0]->GetTeam(), 0);
    for (int i = 0; i < 60; ++i)
    {
        scene.state->Update(kFixedDt);
    }
    EXPECT_GT(scene.state->GetAllUnits()[0]->GetPosition().x, startX + 20.0f);
    IUnitBehaviorHost* host = dynamic_cast<IUnitBehaviorHost*>(scene.state->GetAllUnits()[0]);
    ASSERT_NE(host, nullptr);
    ASSERT_NE(host->GetCurrentBehavior(), nullptr);
    EXPECT_STREQ(host->GetCurrentBehavior()->GetName(), "Seek");
}

TEST(ScenarioTest, CombatPresetSchedulesOneExplosion)
{
    Scene scene = BuildPreset(Preset::Combat, 1);
    ASSERT_EQ(scene.state->GetAllUnits().size(), 4u);
    auto events = PendingEvents(Preset::Combat, *scene.state);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_FLOAT_EQ(events[0].time, 0.5f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetRadius(), 90.0f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetStrength(), 8000.0f);
}

TEST(ScenarioTest, CombatPresetFiresExplosionAndDisplacesUnits)
{
    FrameLoop loop(Preset::Combat, 1, 1280, 720);
    const float spawnX = loop.State().GetAllUnits()[0]->GetPosition().x;
    EXPECT_FLOAT_EQ(spawnX, 360.0f);
    for (int i = 0; i < 20; ++i)
    {
        loop.AdvancePresentationFrame();
    }
    EXPECT_FLOAT_EQ(loop.Engine().GetGameTime(), 20.0f * kFixedDt);
    EXPECT_GT(std::abs(loop.State().GetAllUnits()[0]->GetPosition().x - spawnX), 2.5f);
}

TEST(ScenarioTest, ParsePresetNamesAreLiteral)
{
    Preset preset = Preset::TwoUnits;
    EXPECT_TRUE(ParsePresetFromString("collision", preset));
    EXPECT_EQ(preset, Preset::Collision);
    EXPECT_STREQ(PresetToCliName(Preset::Ai), "ai");
    EXPECT_FALSE(ParsePresetFromString("not-a-preset", preset));
}

TEST(ScenarioTest, ScriptPresetRebuildsCollisionFixture)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("preset collision\n", error), error));
    EXPECT_EQ(loop.State().GetAllUnits().size(), 2u);
    EXPECT_FLOAT_EQ(loop.State().GetAllUnits()[0]->GetPosition().x, 380.0f);
}
