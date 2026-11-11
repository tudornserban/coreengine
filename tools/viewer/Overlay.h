#pragma once

#include "Camera.h"
#include "DrawList.h"
#include "Scenario.h"

#include "core/collision/CollisionEvent.h"
#include "core/collision/ICollisionObserver.h"
#include "core/gamestate/RTSGameState.h"
#include "core/gameloop/GameLogicEngine.h"
#include "core/gameloop/PlaybackController.h"
#include "core/math/Vector2.h"

#include <cstdint>
#include <vector>

struct OverlayFlags
{
    bool collisions = false;
    bool perception = false;
    bool influence = false;
    bool obstacles = false;
    bool behaviorItems = false;
    bool explosions = false;
};

class CollisionRecorder : public ICollisionObserver
{
public:
    void OnCollision(const CollisionEvent& event) override;
    void ExpireOlderThan(float gameTime, float keepSeconds);
    void SetReferenceGameTime(float gameTime) { referenceGameTime_ = gameTime; }
    void Clear();
    const std::vector<CollisionEvent>& Events() const { return events_; }

private:
    std::vector<CollisionEvent> events_;
    std::vector<float> eventTimes_;
    float referenceGameTime_ = 0.0f;
};

struct ExplosionEcho
{
    Vector2 position;
    float radius;
    float secondsLeft;
};

constexpr float kExplosionEchoLifetimeSeconds = 0.5f;
constexpr float kCollisionOverlayKeepSeconds = 3.0f;

constexpr Rgba kViewerCollisionOverlayColor{255, 160, 40, 255};
constexpr Rgba kViewerPerceptionOutlineColor{120, 220, 255, 255};
constexpr Rgba kViewerExplosionOutlineColor{255, 80, 40, 255};
constexpr Rgba kViewerObstacleOverlayColor{160, 160, 160, 255};
constexpr Rgba kViewerHollowCircleFill{0, 0, 0, 0};

void NoteExplosions(const RTSGameState& state, std::vector<ExplosionEcho>& echoes, float dt);
void TickExplosionEchoes(std::vector<ExplosionEcho>& echoes, float dt);

void AppendOverlays(
    DrawList& list,
    RTSGameState& state,
    const Camera& camera,
    const OverlayFlags& flags,
    const CollisionRecorder& collisions,
    const std::vector<ExplosionEcho>& echoes);

void AppendStatusLine(
    DrawList& list,
    const GameLogicEngine& engine,
    const PlaybackController& playback,
    const OverlayFlags& flags,
    std::size_t unitCount);

bool SetOverlayFlag(OverlayFlags& flags, const std::string& name, bool enabled, std::string& error);
bool ToggleOverlayFlag(OverlayFlags& flags, const std::string& name);

enum class OverlayKey
{
    Collisions,
    Perception,
    Influence,
    Obstacles,
    BehaviorItems,
    Explosions,
    Unknown
};

OverlayKey OverlayKeyFromViewerFKey(int keyIndex);
