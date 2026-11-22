#include "platform/unreal/UEMinimapSystem.h"
#include "core/map/MapComponents/MapObstacleComponent.h"
#include "core/map/MapComponents/MapResourceComponent.h"

#include <algorithm>
#include <cmath>

namespace CoreEngine {
namespace Unreal {

Vector2 UEMinimapSystem::WorldCoreToMinimapUV(Vector2 corePos, Vector2 mapSize) {
    const float w = (mapSize.x > 1e-4f) ? mapSize.x : 1.0f;
    const float h = (mapSize.y > 1e-4f) ? mapSize.y : 1.0f;
    const float u = std::max(0.0f, std::min(1.0f, corePos.x / w));
    const float v = std::max(0.0f, std::min(1.0f, corePos.y / h));
    return Vector2(u, v);
}

Vector2 UEMinimapSystem::MinimapUVToWorldCore(Vector2 uv, Vector2 mapSize) {
    const float u = std::max(0.0f, std::min(1.0f, uv.x));
    const float v = std::max(0.0f, std::min(1.0f, uv.y));
    return Vector2(u * mapSize.x, v * mapSize.y);
}

Vector2 UEMinimapSystem::MinimapUVToScreenPixels(Vector2 uv) const {
    return Vector2(
        config_.screenOriginX + uv.x * config_.screenWidthPx,
        config_.screenOriginY + uv.y * config_.screenHeightPx);
}

bool UEMinimapSystem::ScreenPixelsToMinimapUV(Vector2 screenPx, Vector2* outUV) const {
    if (!outUV || config_.screenWidthPx <= 0.0f || config_.screenHeightPx <= 0.0f) {
        return false;
    }
    const float relX = screenPx.x - config_.screenOriginX;
    const float relY = screenPx.y - config_.screenOriginY;
    if (relX < 0.0f || relX > config_.screenWidthPx || relY < 0.0f || relY > config_.screenHeightPx) {
        return false;
    }
    *outUV = Vector2(relX / config_.screenWidthPx, relY / config_.screenHeightPx);
    return true;
}

uint64_t UEMinimapSystem::AddPing(Vector2 corePos, Vector2 mapSize, int team,
                                  const std::string& label, float durationSeconds) {
    UEMinimapPing ping;
    ping.pingId = nextPingId_++;
    ping.team = team;
    ping.worldCorePos = corePos;
    ping.uv = WorldCoreToMinimapUV(corePos, mapSize);
    ping.label = label;
    ping.color = UEColor::ForTeam(team);
    ping.totalDuration = std::max(0.1f, durationSeconds);
    ping.timeRemaining = ping.totalDuration;
    pings_.push_back(ping);
    return ping.pingId;
}

Vector2 UEMinimapSystem::HandleLeftClickUV(Vector2 uv, Vector2 mapSize) {
    cameraCenterCore_ = MinimapUVToWorldCore(uv, mapSize);
    return cameraCenterCore_;
}

bool UEMinimapSystem::HandleRightClickUV(Vector2 uv, RTSGameState& gameState) {
    std::vector<IGameUnit*> selected = gameState.GetSelectionSystem().GetSelectedUnits();
    if (selected.empty()) {
        for (IGameUnit* u : gameState.GetAllUnits()) {
            if (u && u->IsSelected()) {
                selected.push_back(u);
            }
        }
    }
    if (selected.empty()) {
        return false;
    }
    const Vector2 targetCore = MinimapUVToWorldCore(uv, gameState.GetMapSize());
    gameState.GetCommandSystem().MoveUnitsTo(selected, targetCore);
    return true;
}

const UEMinimapFrame& UEMinimapSystem::UpdateFrame(const RTSGameState& gameState,
                                                   const TerritoryCalculator* territory,
                                                   const UECoordinateBridge& bridge,
                                                   float deltaTime) {
    lastFrame_ = UEMinimapFrame{};
    lastFrame_.simulationTick = gameState.GetSimulationTick();
    const Vector2 mapSize = gameState.GetMapSize();
    const float maxDim = std::max(1.0f, std::max(mapSize.x, mapSize.y));

    // Age and prune pings
    if (deltaTime > 0.0f) {
        for (auto& p : pings_) {
            p.timeRemaining -= deltaTime;
        }
    }
    pings_.erase(
        std::remove_if(pings_.begin(), pings_.end(),
                       [](const UEMinimapPing& p) { return p.timeRemaining <= 0.0f; }),
        pings_.end());
    lastFrame_.activePings = pings_;

    // Camera viewport rect
    if (config_.showCameraViewport) {
        const Vector2 halfView = cameraViewSizeCore_ * 0.5f;
        lastFrame_.cameraMinUV = WorldCoreToMinimapUV(cameraCenterCore_ - halfView, mapSize);
        lastFrame_.cameraMaxUV = WorldCoreToMinimapUV(cameraCenterCore_ + halfView, mapSize);
    }

    // Territory heatmap
    const auto allUnits = gameState.GetAllUnits();
    if (territory) {
        const auto calcRes = territory->Calculate(
            allUnits, TerritoryCalculator::Arena{mapSize.x, mapSize.y});
        lastFrame_.team1TerritoryPct = calcRes.team1Pct;
        lastFrame_.team2TerritoryPct = calcRes.team2Pct;

        if (config_.showTerritoryHeatmap && territory->GridCols() > 0 && territory->GridRows() > 0) {
            const int cols = territory->GridCols();
            const int rows = territory->GridRows();
            const auto& grid = territory->GetGrid();
            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    const std::size_t idx = static_cast<std::size_t>(r * cols + c);
                    if (idx >= grid.size()) {
                        continue;
                    }
                    const auto& cell = grid[idx];
                    if (cell.owner == 0 && cell.state == TerritoryCalculator::TerritoryCellState::Neutral) {
                        continue;
                    }
                    UEMinimapTerritoryCell mCell;
                    mCell.col = c;
                    mCell.row = r;
                    mCell.minUV = Vector2(static_cast<float>(c) / static_cast<float>(cols),
                                          static_cast<float>(r) / static_cast<float>(rows));
                    mCell.maxUV = Vector2(static_cast<float>(c + 1) / static_cast<float>(cols),
                                          static_cast<float>(r + 1) / static_cast<float>(rows));
                    mCell.ownerTeam = cell.owner;
                    mCell.state = cell.state;
                    mCell.captureProgress = cell.captureProgress;
                    mCell.color = UEColor::ForTeam(cell.owner);
                    mCell.color.a = (cell.state == TerritoryCalculator::TerritoryCellState::Active) ? 140 : 80;
                    lastFrame_.territoryCells.push_back(mCell);
                }
            }
        }
    }

    // Map obstacles & resources
    if (const Map* map = gameState.GetMap()) {
        for (const auto& entityPtr : map->GetEntities()) {
            if (!entityPtr) {
                continue;
            }
            const Vector2 uv = WorldCoreToMinimapUV(entityPtr->GetPosition(), mapSize);
            for (const auto& compPtr : entityPtr->GetComponents()) {
                if (config_.showObstacles) {
                    if (const auto* obs = dynamic_cast<const MapObstacleComponent*>(compPtr.get())) {
                        UEMinimapBlip b;
                        b.uv = uv;
                        b.radiusUV = std::max(0.015f, obs->GetRadius() / maxDim);
                        b.color = UEColor::ObstacleSlate();
                        b.label = "Obstacle";
                        lastFrame_.obstacleBlips.push_back(b);
                    }
                }
                if (config_.showResources) {
                    if (const auto* res = dynamic_cast<const MapResourceComponent*>(compPtr.get())) {
                        if (!res->IsDepleted()) {
                            UEMinimapBlip b;
                            b.uv = uv;
                            b.radiusUV = 0.022f;
                            b.color = UEColor::Team4Gold();
                            b.label = "Resource";
                            lastFrame_.resourceBlips.push_back(b);
                        }
                    }
                }
            }
        }
    }

    // Units
    for (IGameUnit* unit : allUnits) {
        if (!unit) {
            continue;
        }
        const uint64_t entityId = gameState.GetEntityId(unit);
        const RTSUnit* rtsUnit = dynamic_cast<const RTSUnit*>(unit);

        UEMinimapBlip blip;
        blip.entityId = entityId;
        blip.team = unit->GetTeam();
        blip.uv = WorldCoreToMinimapUV(unit->GetPosition(), mapSize);
        blip.radiusUV = std::max(0.016f, unit->GetRadius() / maxDim);
        blip.headingYawDegrees = bridge.ComputeHeadingYawDegrees(unit->GetVelocity(), 0.0f);
        blip.isSelected = unit->IsSelected();
        blip.isHeroCharacter = (heroEntityIds_.find(entityId) != heroEntityIds_.end());
        if (blip.isHeroCharacter) {
            blip.radiusUV *= 1.4f;
        }
        if (rtsUnit) {
            blip.isDead = rtsUnit->IsDead();
            blip.hpFraction = (rtsUnit->GetMaxHP() > 0.0f)
                ? (rtsUnit->GetCurrentHP() / rtsUnit->GetMaxHP())
                : 0.0f;
            blip.label = rtsUnit->GetStateMachine().GetCurrentState();
        }
        blip.color = blip.isDead
            ? UEColor::DeadDark()
            : (blip.isSelected ? UEColor::SelectedGold() : UEColor::ForTeam(blip.team));
        lastFrame_.unitBlips.push_back(std::move(blip));
    }

    // Projectiles
    if (config_.showProjectiles) {
        for (const auto& proj : gameState.GetProjectiles().Projectiles()) {
            UEMinimapBlip pBlip;
            pBlip.entityId = proj.id;
            pBlip.team = proj.team;
            pBlip.uv = WorldCoreToMinimapUV(proj.position, mapSize);
            pBlip.radiusUV = 0.01f;
            pBlip.color = UEColor::ForTeam(proj.team);
            pBlip.label = "Projectile";
            lastFrame_.projectileBlips.push_back(pBlip);
        }
    }

    return lastFrame_;
}

} // namespace Unreal
} // namespace CoreEngine
