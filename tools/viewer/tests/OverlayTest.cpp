#include "Camera.h"
#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "Overlay.h"
#include "Raster.h"
#include "Scenario.h"
#include "SnapshotRenderer.h"
#include "ViewerInput.h"

#include "core/ai/Behaviors/SeekBehavior.h"
#include "core/combat/Explosion.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/influence_types/InfluenceFieldParameters.h"
#include "core/interfaces/IBehavior.h"
#include "core/interfaces/IUnitKinematics.h"

#include <cmath>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>

namespace
{

class SingleFieldItemBehavior : public IBehavior
{
public:
    explicit SingleFieldItemBehavior(Vector2 position)
        : position_(position)
    {
    }

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext&) override
    {
        return {InfluenceFieldItem::CreateAttractor(position_, 1.0f)};
    }

    const char* GetName() const override { return "SingleFieldItem"; }
    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    Vector2 position_;
    std::map<std::string, float> params_;
};

Camera MakeOffsetCamera()
{
    Camera camera;
    camera.viewportW = 640;
    camera.viewportH = 480;
    camera.center = Vector2(350.0f, 280.0f);
    camera.pixelsPerUnit = 2.25f;
    return camera;
}

bool RgbaEqual(Rgba a, Rgba b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

void ExpectRgbaLiteral(const Rgba& color, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    EXPECT_EQ(color.r, r);
    EXPECT_EQ(color.g, g);
    EXPECT_EQ(color.b, b);
    EXPECT_EQ(color.a, a);
}

int CountPixels(const Framebuffer& fb, Rgba color)
{
    int count = 0;
    for (int y = 0; y < fb.height; ++y)
    {
        for (int x = 0; x < fb.width; ++x)
        {
            const size_t i = static_cast<size_t>((y * fb.width + x) * 4);
            Rgba px{fb.rgba[i], fb.rgba[i + 1], fb.rgba[i + 2], fb.rgba[i + 3]};
            if (RgbaEqual(px, color))
            {
                ++count;
            }
        }
    }
    return count;
}

bool RunScript(FrameLoop& loop, const std::string& text)
{
    std::string error;
    const std::vector<ScriptCommand> commands = ParseScript(text, error);
    if (!error.empty())
    {
        return false;
    }
    return loop.ExecuteScript(commands, error);
}

const DrawCircle* FindExplosionCircle(const DrawList& list)
{
    for (const DrawCircle& circle : list.circles)
    {
        if (circle.outline.r == 255 && circle.outline.g == 80 && circle.outline.b == 40 && circle.outline.a == 255
            && circle.fill.r == 0 && circle.fill.g == 0 && circle.fill.b == 0 && circle.fill.a == 0)
        {
            return &circle;
        }
    }
    return nullptr;
}

} // namespace

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

TEST(OverlayTest, CollisionOverlayProjectsCrossAndNormal)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    const Camera camera = MakeOffsetCamera();
    CollisionRecorder recorder;
    CollisionEvent event;
    event.contactPoint = Vector2(220.0f, 180.0f);
    event.normal = Vector2(0.0f, 1.0f);
    recorder.OnCollision(event);

    OverlayFlags flags;
    flags.collisions = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, recorder, {});
    ASSERT_EQ(list.lines.size(), 3u);
    for (const DrawLine& line : list.lines)
    {
        ExpectRgbaLiteral(line.color, 255, 160, 40, 255);
    }
    const Vector2 contactScreen = camera.WorldToScreen(event.contactPoint);
    EXPECT_NEAR(list.lines[0].a.x, contactScreen.x - 3.0f, 0.01f);
    const Vector2 normalScreen = camera.WorldToScreen(event.contactPoint + event.normal * 20.0f);
    EXPECT_NEAR(list.lines[2].b.y, normalScreen.y, 0.01f);
}

TEST(OverlayTest, PerceptionRingUsesCameraAndLargeRadius)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    scene.state->SetPerceptionRadius(1000.0f);
    const Camera camera = MakeOffsetCamera();
    OverlayFlags flags;
    flags.perception = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});

    IGameUnit* unit = scene.state->GetAllUnits()[0];
    const Vector2 unitScreen = camera.WorldToScreen(unit->GetPosition());
    ASSERT_EQ(list.circles.size(), 2u);
    EXPECT_NEAR(list.circles[0].radius, 1000.0f * camera.pixelsPerUnit, 0.01f);
    ExpectRgbaLiteral(list.circles[0].outline, 120, 220, 255, 255);
    ExpectRgbaLiteral(list.circles[0].fill, 0, 0, 0, 0);

    scene.state->SetPerceptionRadius(45.0f);
    list.circles.clear();
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});
    const float rasterRing = list.circles[0].radius;
    Framebuffer fb;
    fb.width = camera.viewportW;
    fb.height = camera.viewportH;
    DrawList base = BuildSnapshotDrawList(*scene.state, camera);
    Rasterize(base, fb);
    const int cx = static_cast<int>(std::lround(unitScreen.x));
    const int cy = static_cast<int>(std::lround(unitScreen.y));
    const size_t centerIndex = static_cast<size_t>((cy * fb.width + cx) * 4);
    const Rgba centerBefore{fb.rgba[centerIndex], fb.rgba[centerIndex + 1], fb.rgba[centerIndex + 2], fb.rgba[centerIndex + 3]};
    list = base;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});
    Rasterize(list, fb);
    const Rgba centerAfter{fb.rgba[centerIndex], fb.rgba[centerIndex + 1], fb.rgba[centerIndex + 2], fb.rgba[centerIndex + 3]};
    EXPECT_TRUE(RgbaEqual(centerBefore, centerAfter));

    bool foundRingPixel = false;
    for (int y = 0; y < fb.height; ++y)
    {
        for (int x = 0; x < fb.width; ++x)
        {
            const size_t i = static_cast<size_t>((y * fb.width + x) * 4);
            Rgba px{fb.rgba[i], fb.rgba[i + 1], fb.rgba[i + 2], fb.rgba[i + 3]};
            if (px.r != 120 || px.g != 220 || px.b != 255 || px.a != 255)
            {
                continue;
            }
            const float dx = static_cast<float>(x) - unitScreen.x;
            const float dy = static_cast<float>(y) - unitScreen.y;
            if (std::fabs(std::sqrt(dx * dx + dy * dy) - rasterRing) <= 2.0f)
            {
                foundRingPixel = true;
                break;
            }
        }
        if (foundRingPixel)
        {
            break;
        }
    }
    EXPECT_TRUE(foundRingPixel);
}

TEST(OverlayTest, NoteExplosionsSetsLifetimeAndTickDecays)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    scene.state->AddExplosion(Explosion(Vector2(10, 10), 5.0f, 1.0f));
    std::vector<ExplosionEcho> echoes;
    NoteExplosions(*scene.state, echoes, kFixedDt);
    ASSERT_EQ(echoes.size(), 1u);
    EXPECT_FLOAT_EQ(echoes[0].secondsLeft, kExplosionEchoLifetimeSeconds);
    TickExplosionEchoes(echoes, kFixedDt);
    ASSERT_EQ(echoes.size(), 1u);
    EXPECT_NEAR(echoes[0].secondsLeft, kExplosionEchoLifetimeSeconds - kFixedDt, 0.0001f);
}

TEST(OverlayTest, FrameLoopStepNotesExplosionBeforeUpdate)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    const Vector2 explosionPos(400.0f, 300.0f);
    const float explosionRadius = 80.0f;
    loop.State().AddExplosion(Explosion(explosionPos, explosionRadius, 5000.0f));
    ASSERT_TRUE(RunScript(loop, "overlay explosions 1\npause\nstep\n"));
    EXPECT_TRUE(loop.State().GetExplosions().empty());
    const Camera camera = loop.GetCamera();
    const DrawList drawList = loop.BuildViewDrawList();
    const DrawCircle* ring = FindExplosionCircle(drawList);
    ASSERT_NE(ring, nullptr);
    EXPECT_NEAR(ring->center.x, camera.WorldToScreen(explosionPos).x, 0.01f);
    EXPECT_NEAR(ring->radius, explosionRadius * camera.pixelsPerUnit, 0.01f);
    ExpectRgbaLiteral(ring->outline, 255, 80, 40, 255);
}

TEST(OverlayTest, PausedFramesDoNotReNoteExplosion)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    loop.State().AddExplosion(Explosion(Vector2(400.0f, 300.0f), 80.0f, 5000.0f));
    ASSERT_TRUE(RunScript(loop, "overlay explosions 1\npause\nframes 40\n"));
    EXPECT_EQ(loop.State().GetExplosions().size(), 1u);
    EXPECT_EQ(FindExplosionCircle(loop.BuildViewDrawList()), nullptr);
}

TEST(OverlayTest, FrameLoopEchoVisibleThenExpiresAfterSimFrames)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    loop.State().AddExplosion(Explosion(Vector2(400.0f, 300.0f), 80.0f, 5000.0f));
    ASSERT_TRUE(RunScript(loop, "overlay explosions 1\nframes 1\n"));
    EXPECT_NE(FindExplosionCircle(loop.BuildViewDrawList()), nullptr);

    FrameLoop loopFresh(Preset::TwoUnits, 1, 1280, 720);
    loopFresh.State().AddExplosion(Explosion(Vector2(400.0f, 300.0f), 80.0f, 5000.0f));
    ASSERT_TRUE(RunScript(loopFresh, "overlay explosions 1\nframes 20\n"));
    EXPECT_EQ(FindExplosionCircle(loopFresh.BuildViewDrawList()), nullptr);
}

TEST(OverlayTest, CollisionOverlayExpiresAfterSimulatedTime)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    auto unitA = std::make_unique<RTSUnit>(Vector2(390.0f, 300.0f), 1, 12.0f, 100.0f);
    auto unitB = std::make_unique<RTSUnit>(Vector2(400.0f, 300.0f), 2, 12.0f, 100.0f);
    dynamic_cast<IUnitKinematics*>(unitA.get())->SetVelocity(Vector2(80.0f, 0.0f));
    dynamic_cast<IUnitKinematics*>(unitB.get())->SetVelocity(Vector2(-80.0f, 0.0f));
    loop.State().AddUnit(std::move(unitA));
    loop.State().AddUnit(std::move(unitB));
    ASSERT_TRUE(RunScript(loop, "overlay collisions 1\nframes 1\n"));
    Framebuffer afterHit;
    loop.RasterizeCurrentView(afterHit);
    EXPECT_GT(CountPixels(afterHit, {255, 160, 40, 255}), 0);
    int spread = 0;
    for (IGameUnit* unit : loop.State().GetAllUnits())
    {
        if (auto* kin = dynamic_cast<IUnitKinematics*>(unit))
        {
            kin->SetVelocity(Vector2(0.0f, 0.0f));
            kin->SetPosition(Vector2(50.0f + static_cast<float>(spread) * 180.0f, 50.0f));
            ++spread;
        }
    }
    ASSERT_TRUE(RunScript(loop, "frames 100\n"));
    Framebuffer expired;
    loop.RasterizeCurrentView(expired);
    EXPECT_EQ(0, CountPixels(expired, {255, 160, 40, 255}));
}

TEST(OverlayTest, InfluenceDrawsEdgeAndAttractorRepulsorMarkers)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    const Camera camera = MakeOffsetCamera();
    const auto& edges = scene.state->GetStaticItems();
    scene.state->AddStaticAttractor(InfluenceFieldItem::CreateAttractor(Vector2(100.0f, 150.0f), 1.0f));
    scene.state->AddStaticAttractor(InfluenceFieldItem::CreateRepulsor(Vector2(500.0f, 400.0f), 1.0f));
    OverlayFlags flags;
    flags.influence = true;
    DrawList list;
    AppendOverlays(list, *scene.state, camera, flags, CollisionRecorder{}, {});
    ASSERT_EQ(list.circles.size(), 6u);
    const float markerPx = 6.0f * camera.pixelsPerUnit;
    for (size_t i = 0; i < 4u; ++i)
    {
        ExpectRgbaLiteral(list.circles[i].fill, 0, 220, 220, 255);
        EXPECT_NEAR(list.circles[i].center.x, camera.WorldToScreen(edges[i].GetPosition()).x, 0.01f);
        EXPECT_NEAR(list.circles[i].radius, markerPx, 0.01f);
    }
    ExpectRgbaLiteral(list.circles[4].fill, 80, 220, 80, 255);
    ExpectRgbaLiteral(list.circles[5].fill, 220, 60, 60, 255);
}

TEST(OverlayTest, ObstacleOverlayUsesEntityPositionAndRadius)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    const Vector2 obstaclePos(250.0f, 175.0f);
    const float obstacleRadius = 22.0f;
    RepulsorParams params;
    params.DetectionRange = obstacleRadius;
    scene.state->AddObstacle(InfluenceFieldItem::CreateRepulsor(obstaclePos, 1.0f, params));
    const Camera camera = MakeOffsetCamera();
    OverlayFlags flags;
    flags.obstacles = true;
    DrawList listOn;
    AppendOverlays(listOn, *scene.state, camera, flags, CollisionRecorder{}, {});
    ASSERT_EQ(listOn.circles.size(), 1u);
    ExpectRgbaLiteral(listOn.circles[0].fill, 160, 160, 160, 255);
    ExpectRgbaLiteral(listOn.circles[0].outline, 160, 160, 160, 255);
    const Vector2 expectedCenter = camera.WorldToScreen(obstaclePos);
    EXPECT_NEAR(listOn.circles[0].center.x, expectedCenter.x, 0.01f);
    EXPECT_NEAR(listOn.circles[0].center.y, expectedCenter.y, 0.01f);
    EXPECT_NEAR(listOn.circles[0].radius, obstacleRadius * camera.pixelsPerUnit, 0.01f);
}

TEST(OverlayTest, BehaviorOverlayEmptyForSeekAndDrawsEmittedItem)
{
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    IGameUnit* unit = scene.state->GetAllUnits()[0];
    auto* rts = dynamic_cast<RTSUnit*>(unit);
    ASSERT_NE(rts, nullptr);
    rts->GetStateMachine().AddState("Seek", std::make_unique<SeekBehavior>(Vector2(700.0f, 300.0f)));
    rts->GetStateMachine().SetState("Seek");
    OverlayFlags flags;
    flags.behaviorItems = true;
    DrawList list;
    AppendOverlays(list, *scene.state, FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f), flags, CollisionRecorder{}, {});
    EXPECT_TRUE(list.circles.empty());
    const Vector2 emitPos(510.0f, 220.0f);
    rts->GetStateMachine().AddState("Emit", std::make_unique<SingleFieldItemBehavior>(emitPos));
    rts->GetStateMachine().SetState("Emit");
    list.circles.clear();
    const Camera offsetCamera = MakeOffsetCamera();
    AppendOverlays(list, *scene.state, offsetCamera, flags, CollisionRecorder{}, {});
    ASSERT_EQ(list.circles.size(), 1u);
    const Vector2 expectedCenter = offsetCamera.WorldToScreen(emitPos);
    EXPECT_NEAR(list.circles[0].center.x, expectedCenter.x, 0.01f);
    EXPECT_NEAR(list.circles[0].center.y, expectedCenter.y, 0.01f);
    ExpectRgbaLiteral(list.circles[0].fill, 80, 220, 80, 255);
}

TEST(OverlayTest, ScriptTogglesEveryOverlayFlag)
{
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    struct FlagCase
    {
        const char* name;
        bool OverlayFlags::* member;
    };
    const FlagCase cases[] = {
        {"collisions", &OverlayFlags::collisions},
        {"perception", &OverlayFlags::perception},
        {"influence", &OverlayFlags::influence},
        {"obstacles", &OverlayFlags::obstacles},
        {"behavior", &OverlayFlags::behaviorItems},
        {"explosions", &OverlayFlags::explosions},
    };
    for (const FlagCase& flagCase : cases)
    {
        ASSERT_TRUE(RunScript(loop, std::string("overlay ") + flagCase.name + " 1\n"));
        EXPECT_TRUE(loop.GetOverlayFlags().*flagCase.member);
        ASSERT_TRUE(RunScript(loop, std::string("overlay ") + flagCase.name + " 0\n"));
        EXPECT_FALSE(loop.GetOverlayFlags().*flagCase.member);
    }
}

TEST(OverlayTest, HandleViewerKeycodeFKeysToggleOverlayFlags)
{
    constexpr int kSdlF1 = 1073741882;
    FrameLoop loop(Preset::TwoUnits, 1, 1280, 720);
    struct ToggleCase
    {
        int keycode;
        int expectedIndex;
        bool OverlayFlags::* member;
    };
    const ToggleCase cases[] = {
        {kSdlF1 + 0, 1, &OverlayFlags::collisions},
        {kSdlF1 + 1, 2, &OverlayFlags::perception},
        {kSdlF1 + 2, 3, &OverlayFlags::influence},
        {kSdlF1 + 3, 4, &OverlayFlags::obstacles},
        {kSdlF1 + 4, 5, &OverlayFlags::behaviorItems},
        {kSdlF1 + 5, 6, &OverlayFlags::explosions},
    };
    for (const ToggleCase& toggleCase : cases)
    {
        const ViewerKeyAction action = HandleViewerKeycode(toggleCase.keycode);
        ASSERT_EQ(action.kind, ViewerKeyActionKind::OverlayToggle);
        EXPECT_EQ(action.overlayToggleIndex, toggleCase.expectedIndex);
        loop.ApplyViewerKeyAction(action);
        EXPECT_TRUE(loop.GetOverlayFlags().*toggleCase.member);
        loop.ApplyViewerKeyAction(action);
        EXPECT_FALSE(loop.GetOverlayFlags().*toggleCase.member);
    }
}

TEST(OverlayTest, AppendStatusLinePinsTextLayout)
{
    GameLogicEngine engine(kFixedDt);
    PlaybackController playback(&engine);
    OverlayFlags flags;
    flags.collisions = true;
    DrawList list;
    AppendStatusLine(list, engine, playback, flags, 2);
    ASSERT_FALSE(list.rects.empty());
    EXPECT_NEAR(list.rects[0].min.x, 8.0f, 0.01f);
    DrawList changed;
    playback.Pause();
    AppendStatusLine(changed, engine, playback, flags, 2);
    EXPECT_NE(list.rects.size(), changed.rects.size());
}

TEST(OverlayTest, ExplosionOutlineColorPinned)
{
    std::vector<ExplosionEcho> echoes{{Vector2(1, 1), 10.0f, 0.5f}};
    DrawList list;
    Scene scene = BuildPreset(Preset::TwoUnits, 1);
    OverlayFlags flags;
    flags.explosions = true;
    AppendOverlays(list, *scene.state, FitMap(scene.state->GetMapSize(), 1280, 720, 40.0f), flags, CollisionRecorder{}, echoes);
    ASSERT_EQ(list.circles.size(), 1u);
    ExpectRgbaLiteral(list.circles[0].outline, 255, 80, 40, 255);
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
