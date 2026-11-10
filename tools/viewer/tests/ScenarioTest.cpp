#include "DrawList.h"
#include "FrameLoop.h"
#include "Overlay.h"
#include "Scenario.h"
#include "SnapshotRenderer.h"

#include "core/collision/ICollisionObserver.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IUnitBehaviorHost.h"
#include "core/interfaces/IUnitKinematics.h"

#include <cmath>
#include <gtest/gtest.h>

namespace
{

int CountExplosionOutlineCircles(const DrawList& list)
{
    int count = 0;
    for (const DrawCircle& circle : list.circles)
    {
        if (circle.outline.r == 255 && circle.outline.g == 80 && circle.outline.b == 40
            && circle.outline.a == 255)
        {
            ++count;
        }
    }
    return count;
}

IUnitBehaviorHost* BehaviorHost(IGameUnit* unit)
{
    return dynamic_cast<IUnitBehaviorHost*>(unit);
}

void ExpectBehaviorName(IGameUnit* unit, const char* literalName)
{
    IUnitBehaviorHost* host = BehaviorHost(unit);
    ASSERT_NE(host, nullptr);
    ASSERT_NE(host->GetCurrentBehavior(), nullptr);
    EXPECT_STREQ(host->GetCurrentBehavior()->GetName(), literalName);
}

} // namespace

TEST(ScenarioTest, ParsePresetMapsAllCliNamesToEnums)
{
    Preset preset = Preset::TwoUnits;
    ASSERT_TRUE(ParsePresetFromString("two-units", preset));
    EXPECT_EQ(preset, Preset::TwoUnits);
    ASSERT_TRUE(ParsePresetFromString("collision", preset));
    EXPECT_EQ(preset, Preset::Collision);
    ASSERT_TRUE(ParsePresetFromString("combat", preset));
    EXPECT_EQ(preset, Preset::Combat);
    ASSERT_TRUE(ParsePresetFromString("ai", preset));
    EXPECT_EQ(preset, Preset::Ai);
    EXPECT_FALSE(ParsePresetFromString("not-a-preset", preset));
}

TEST(ScenarioTest, CollisionPresetSpawnAndVelocityLiterals)
{
    Scene scene = BuildPreset(Preset::Collision, 1);
    ASSERT_EQ(scene.state->GetAllUnits().size(), 2u);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetPosition().x, 380.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetPosition().y, 300.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[1]->GetPosition().x, 420.0f);
    EXPECT_EQ(scene.state->GetAllUnits()[0]->GetTeam(), 1);
    EXPECT_EQ(scene.state->GetAllUnits()[1]->GetTeam(), 2);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetRadius(), 16.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetVelocity().x, 80.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[0]->GetVelocity().y, 0.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[1]->GetVelocity().x, -80.0f);
    EXPECT_FLOAT_EQ(scene.state->GetAllUnits()[1]->GetVelocity().y, 0.0f);
    EXPECT_FLOAT_EQ(scene.state->GetPerceptionRadius(), 80.0f);
}

TEST(ScenarioTest, CollisionPresetOverlapsAndRecordsAContact)
{
    Scene scene = BuildPreset(Preset::Collision, 1);
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

TEST(ScenarioTest, CombatPresetSpawnVelocityAndPerceptionLiterals)
{
    Scene scene = BuildPreset(Preset::Combat, 1);
    const auto& units = scene.state->GetAllUnits();
    ASSERT_EQ(units.size(), 4u);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().x, 360.0f);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().y, 280.0f);
    EXPECT_EQ(units[0]->GetTeam(), 1);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().x, 360.0f);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().y, 320.0f);
    EXPECT_EQ(units[1]->GetTeam(), 2);
    EXPECT_FLOAT_EQ(units[2]->GetPosition().x, 440.0f);
    EXPECT_FLOAT_EQ(units[2]->GetPosition().y, 280.0f);
    EXPECT_EQ(units[2]->GetTeam(), 1);
    EXPECT_FLOAT_EQ(units[3]->GetPosition().x, 440.0f);
    EXPECT_FLOAT_EQ(units[3]->GetPosition().y, 320.0f);
    EXPECT_EQ(units[3]->GetTeam(), 2);
    EXPECT_FLOAT_EQ(units[0]->GetVelocity().x, 80.0f);
    EXPECT_FLOAT_EQ(units[1]->GetVelocity().x, 80.0f);
    EXPECT_FLOAT_EQ(units[2]->GetVelocity().x, -80.0f);
    EXPECT_FLOAT_EQ(units[3]->GetVelocity().x, -80.0f);
    EXPECT_FLOAT_EQ(scene.state->GetPerceptionRadius(), 80.0f);
}

TEST(ScenarioTest, CombatPresetSchedulesExplosionAtMapCenter)
{
    Scene scene = BuildPreset(Preset::Combat, 1);
    auto events = PendingEvents(Preset::Combat, *scene.state);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_FLOAT_EQ(events[0].time, 0.5f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetPosition().x, 400.0f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetPosition().y, 300.0f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetRadius(), 90.0f);
    EXPECT_FLOAT_EQ(events[0].explosion.GetStrength(), 8000.0f);
}

TEST(ScenarioTest, CombatExplosionOverlayAfterFifteenFrames)
{
    FrameLoop loop(Preset::Combat, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("overlay explosions 1\nframes 15\n", error), error));
    const DrawList list = loop.BuildViewDrawList();
    ASSERT_EQ(CountExplosionOutlineCircles(list), 1);
    const float pixelsPerUnit = 640.0f / 600.0f;
    for (const DrawCircle& circle : list.circles)
    {
        if (circle.outline.r != 255 || circle.outline.g != 80)
        {
            continue;
        }
        EXPECT_NEAR(circle.center.x, 640.0f, 0.05f);
        EXPECT_NEAR(circle.center.y, 360.0f, 0.05f);
        EXPECT_NEAR(circle.radius, 90.0f * pixelsPerUnit, 0.05f);
    }
}

TEST(ScenarioTest, CombatExplosionFiresOnceAfterTwentyFrames)
{
    FrameLoop loop(Preset::Combat, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(
        ParseScript("overlay explosions 1\nframes 20\n", error), error));
    EXPECT_EQ(loop.CountFiredPendingScenarioEvents(), 1);
    EXPECT_EQ(CountExplosionOutlineCircles(loop.BuildViewDrawList()), 1);
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

TEST(ScenarioTest, AiPresetSpawnAndBehaviorLiterals)
{
    Scene scene = BuildPreset(Preset::Ai, 1);
    const auto& units = scene.state->GetAllUnits();
    ASSERT_EQ(units.size(), 4u);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().x, 120.0f);
    EXPECT_FLOAT_EQ(units[0]->GetPosition().y, 300.0f);
    EXPECT_EQ(units[0]->GetTeam(), 0);
    ExpectBehaviorName(units[0], "Seek");
    EXPECT_FLOAT_EQ(units[1]->GetPosition().x, 700.0f);
    EXPECT_FLOAT_EQ(units[1]->GetPosition().y, 450.0f);
    EXPECT_EQ(units[1]->GetTeam(), 1);
    ExpectBehaviorName(units[1], "Flee");
    EXPECT_FLOAT_EQ(units[2]->GetPosition().x, 140.0f);
    EXPECT_FLOAT_EQ(units[2]->GetPosition().y, 260.0f);
    EXPECT_EQ(units[2]->GetTeam(), 0);
    ExpectBehaviorName(units[2], "Separation");
    EXPECT_FLOAT_EQ(units[3]->GetPosition().x, 140.0f);
    EXPECT_FLOAT_EQ(units[3]->GetPosition().y, 340.0f);
    EXPECT_EQ(units[3]->GetTeam(), 0);
    ExpectBehaviorName(units[3], "Separation");
    EXPECT_FLOAT_EQ(scene.state->GetPerceptionRadius(), 80.0f);
}

TEST(ScenarioTest, AiSeekUnitMovesTowardTheTarget)
{
    Scene scene = BuildPreset(Preset::Ai, 1);
    const float startX = scene.state->GetAllUnits()[0]->GetPosition().x;
    EXPECT_FLOAT_EQ(startX, 120.0f);
    for (int i = 0; i < 60; ++i)
    {
        scene.state->Update(kFixedDt);
    }
    EXPECT_GT(scene.state->GetAllUnits()[0]->GetPosition().x, startX + 20.0f);
}

TEST(ScenarioTest, ScriptPresetAiLeavesSeekAtOneTwenty)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(ParseScript("preset ai\n", error), error));
    EXPECT_FLOAT_EQ(loop.State().GetAllUnits()[0]->GetPosition().x, 120.0f);
    ExpectBehaviorName(loop.State().GetAllUnits()[0], "Seek");
}

TEST(ScenarioTest, ScriptPresetCombatLeavesSpawnAndOnePendingEvent)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    std::string error;
    ASSERT_TRUE(loop.ExecuteScript(ParseScript("preset combat\n", error), error));
    EXPECT_FLOAT_EQ(loop.State().GetAllUnits()[0]->GetPosition().x, 360.0f);
    auto events = PendingEvents(Preset::Combat, loop.State());
    ASSERT_EQ(events.size(), 1u);
    EXPECT_FLOAT_EQ(events[0].time, 0.5f);
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
