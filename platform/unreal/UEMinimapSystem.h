#pragma once

#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/influence/TerritoryCalculator.h"
#include "platform/unreal/UECoordinateBridge.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace CoreEngine {
namespace Unreal {

struct UEMinimapConfig {
    float screenOriginX = 980.0f;
    float screenOriginY = 440.0f;
    float screenWidthPx = 260.0f;
    float screenHeightPx = 240.0f;

    bool showTerritoryHeatmap = true;
    bool showObstacles = true;
    bool showResources = true;
    bool showProjectiles = true;
    bool showUnitHeadings = true;
    bool showCameraViewport = true;
};

struct UEMinimapBlip {
    uint64_t entityId = 0;
    int team = 0;
    Vector2 uv{0.0f, 0.0f};
    float radiusUV = 0.02f;
    float headingYawDegrees = 0.0f;
    float hpFraction = 1.0f;
    bool isSelected = false;
    bool isDead = false;
    bool isHeroCharacter = false;
    UEColor color;
    std::string label;
};

struct UEMinimapTerritoryCell {
    int col = 0;
    int row = 0;
    Vector2 minUV{0.0f, 0.0f};
    Vector2 maxUV{0.0f, 0.0f};
    int ownerTeam = 0;
    TerritoryCalculator::TerritoryCellState state = TerritoryCalculator::TerritoryCellState::Neutral;
    float captureProgress = 0.0f;
    UEColor color;
};

struct UEMinimapPing {
    uint64_t pingId = 0;
    int team = 0;
    Vector2 worldCorePos{0.0f, 0.0f};
    Vector2 uv{0.0f, 0.0f};
    std::string label;
    UEColor color;
    float timeRemaining = 3.0f;
    float totalDuration = 3.0f;

    float PulseFraction() const {
        return (totalDuration > 0.0f) ? (1.0f - (timeRemaining / totalDuration)) : 1.0f;
    }
};

struct UEMinimapFrame {
    uint32_t simulationTick = 0;
    float team1TerritoryPct = 0.0f;
    float team2TerritoryPct = 0.0f;

    Vector2 cameraMinUV{0.0f, 0.0f};
    Vector2 cameraMaxUV{1.0f, 1.0f};

    std::vector<UEMinimapTerritoryCell> territoryCells;
    std::vector<UEMinimapBlip> unitBlips;
    std::vector<UEMinimapBlip> obstacleBlips;
    std::vector<UEMinimapBlip> resourceBlips;
    std::vector<UEMinimapBlip> projectileBlips;
    std::vector<UEMinimapPing> activePings;
};

/// Generic Minimap & Tactical Radar system for CoreEngine in UE5.
class UEMinimapSystem {
private:
    UEMinimapConfig config_;
    UEMinimapFrame lastFrame_;
    std::vector<UEMinimapPing> pings_;
    std::unordered_set<uint64_t> heroEntityIds_;
    uint64_t nextPingId_ = 1;
    Vector2 cameraCenterCore_{400.0f, 300.0f};
    Vector2 cameraViewSizeCore_{400.0f, 300.0f};

public:
    explicit UEMinimapSystem(UEMinimapConfig config = UEMinimapConfig{})
        : config_(config) {}

    const UEMinimapConfig& GetConfig() const { return config_; }
    UEMinimapConfig& GetConfig() { return config_; }
    void SetConfig(const UEMinimapConfig& config) { config_ = config; }

    void SetCameraViewportCore(Vector2 centerCore, Vector2 viewSizeCore) {
        cameraCenterCore_ = centerCore;
        cameraViewSizeCore_ = viewSizeCore;
    }
    Vector2 GetCameraCenterCore() const { return cameraCenterCore_; }
    Vector2 GetCameraViewSizeCore() const { return cameraViewSizeCore_; }

    void MarkHeroEntity(uint64_t entityId, bool isHero = true) {
        if (isHero) {
            heroEntityIds_.insert(entityId);
        } else {
            heroEntityIds_.erase(entityId);
        }
    }

    // --- Coordinate Projections ---
    static Vector2 WorldCoreToMinimapUV(Vector2 corePos, Vector2 mapSize);
    static Vector2 MinimapUVToWorldCore(Vector2 uv, Vector2 mapSize);

    Vector2 MinimapUVToScreenPixels(Vector2 uv) const;
    bool ScreenPixelsToMinimapUV(Vector2 screenPx, Vector2* outUV) const;

    // --- Radar Pings ---
    uint64_t AddPing(Vector2 corePos, Vector2 mapSize, int team = 1,
                     const std::string& label = "Alert", float durationSeconds = 3.0f);
    const std::vector<UEMinimapPing>& GetActivePings() const { return pings_; }
    void ClearPings() { pings_.clear(); }

    // --- Minimap Click Commands ---
    /// Left-clicking the minimap pans the 3D camera center to the clicked arena coordinate.
    Vector2 HandleLeftClickUV(Vector2 uv, Vector2 mapSize);

    /// Right-clicking the minimap issues a MoveUnitsTo command to all currently selected units.
    bool HandleRightClickUV(Vector2 uv, RTSGameState& gameState);

    /// Build the latest minimap frame snapshot from RTSGameState and TerritoryCalculator.
    const UEMinimapFrame& UpdateFrame(const RTSGameState& gameState,
                                      const TerritoryCalculator* territory,
                                      const UECoordinateBridge& bridge,
                                      float deltaTime);

    const UEMinimapFrame& GetLastFrame() const { return lastFrame_; }
};

} // namespace Unreal
} // namespace CoreEngine
