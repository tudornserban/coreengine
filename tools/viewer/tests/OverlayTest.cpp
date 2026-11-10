#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Overlay.h"
#include "Raster.h"
#include "Scenario.h"

#include "core/combat/Explosion.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/interfaces/IUnitKinematics.h"

#include <cstring>
#include <gtest/gtest.h>

TEST(OverlayTest, ExplosionEchoSurvivesTheTickThatClearsTheEngine)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    scene.state->AddExplosion(Explosion(Vector2(400, 300), 80.0f, 5000.0f));
    std::vector<ExplosionEcho> echoes;
    NoteExplosions(*scene.state, echoes, kFixedDt);
    ASSERT_EQ(echoes.size(), 1u);
    scene.state->Update(kFixedDt);
    EXPECT_TRUE(scene.state->GetExplosions().empty());
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    OverlayFlags flags;
    flags.explosions = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, echoes);
    ASSERT_EQ(list.circles.size(), 1u);
    EXPECT_NEAR(list.circles[0].radius, 80.0f * camera.pixelsPerUnit, 0.01f);
}

TEST(OverlayTest, FlagsOffAddNoOverlayPrimitives)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    DrawList list;
    AppendOverlays(list, *scene.state, camera, OverlayFlags{}, CollisionRecorder{}, {});
    EXPECT_TRUE(list.circles.empty());
    EXPECT_TRUE(list.lines.empty());
}

TEST(OverlayTest, InfluenceToggleDrawsFourEdgeMarkers)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    OverlayFlags flags;
    flags.influence = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});
    EXPECT_EQ(list.circles.size(), 4u);
}

TEST(OverlayTest, CollisionRecorderCapturesOverlap)
{
    Scene scene;
    scene.state = std::make_unique<RTSGameState>(Vector2(800.0f, 600.0f), 1);
    auto unitA = std::make_unique<RTSUnit>(Vector2(390.0f, 300.0f), 1, 12.0f, 100.0f);
  auto unitB = std::make_unique<RTSUnit>(Vector2(400.0f, 300.0f), 2, 12.0f, 100.0f);
  auto* kinA = dynamic_cast<IUnitKinematics*>(unitA.get());
  auto* kinB = dynamic_cast<IUnitKinematics*>(unitB.get());
  ASSERT_NE(kinA, nullptr);
  ASSERT_NE(kinB, nullptr);
  kinA->SetVelocity(Vector2(50.0f, 0.0f));
  kinB->SetVelocity(Vector2(-50.0f, 0.0f));
  scene.state->AddUnit(std::move(unitA));
  scene.state->AddUnit(std::move(unitB));

  CollisionRecorder recorder;
  scene.state->GetCollisionSystem().Subscribe(&recorder);
  recorder.SetReferenceGameTime(0.0f);
  scene.state->Update(kFixedDt);
  EXPECT_GE(recorder.Events().size(), 1u);
  scene.state->GetCollisionSystem().Unsubscribe(&recorder);
}

TEST(OverlayTest, CollisionOverlayUsesContactCrossAndNormal)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    CollisionRecorder recorder;
    CollisionEvent event;
    event.contactPoint = Vector2(100.0f, 200.0f);
    event.normal = Vector2(1.0f, 0.0f);
    recorder.OnCollision(event);

    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    OverlayFlags flags;
    flags.collisions = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, recorder, {});
    ASSERT_EQ(list.lines.size(), 3u);
    EXPECT_EQ(list.lines[0].color.r, 255);
    EXPECT_EQ(list.lines[0].color.g, 160);
}

TEST(OverlayTest, PerceptionOverlayUsesStateRadiusAndCamera)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    scene.state->SetPerceptionRadius(50.0f);
    Camera camera = FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f);
    OverlayFlags flags;
    flags.perception = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});
    ASSERT_EQ(list.circles.size(), 2u);
    EXPECT_NEAR(list.circles[0].radius, 50.0f * camera.pixelsPerUnit, 0.01f);
}

TEST(OverlayTest, ExplosionEchoLifetimeDecays)
{
    std::vector<ExplosionEcho> echoes;
    echoes.push_back({Vector2(10, 10), 5.0f, 0.1f});
    TickExplosionEchoes(echoes, 0.2f);
    EXPECT_TRUE(echoes.empty());
}

TEST(OverlayTest, FrameLoopScriptTurnsInfluenceOverlayOnAndOff)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    std::string error;
    Framebuffer baseline;
    loop.RasterizeCurrentView(baseline);

    std::vector<ScriptCommand> onCmd;
    ScriptCommand enable;
    enable.kind = ScriptCommand::Kind::Overlay;
    enable.strArg = "influence";
    enable.intArg = 1;
    onCmd.push_back(enable);
    ASSERT_TRUE(loop.ExecuteScript(onCmd, error));
    EXPECT_TRUE(loop.GetOverlayFlags().influence);

    Framebuffer fbOn;
    loop.RasterizeCurrentView(fbOn);
    ASSERT_EQ(baseline.rgba.size(), fbOn.rgba.size());
    EXPECT_NE(0, std::memcmp(baseline.rgba.data(), fbOn.rgba.data(), baseline.rgba.size()));

    std::vector<ScriptCommand> offCmd;
    ScriptCommand disable = enable;
    disable.intArg = 0;
    offCmd.push_back(disable);
    ASSERT_TRUE(loop.ExecuteScript(offCmd, error));
    EXPECT_FALSE(loop.GetOverlayFlags().influence);

    Framebuffer fbOff;
    loop.RasterizeCurrentView(fbOff);
    EXPECT_EQ(0, std::memcmp(baseline.rgba.data(), fbOff.rgba.data(), baseline.rgba.size()));
}

TEST(InputScriptTest, ParsesOverlayCommand)
{
    std::string error;
    const auto cmds = ParseScript("overlay perception 1\n", error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(cmds.size(), 1u);
    EXPECT_EQ(cmds[0].kind, ScriptCommand::Kind::Overlay);
    EXPECT_EQ(cmds[0].strArg, "perception");
    EXPECT_EQ(cmds[0].intArg, 1);
}
