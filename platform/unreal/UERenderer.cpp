#include "platform/unreal/UERenderer.h"
#include "core/map/MapComponents/MapObstacleComponent.h"

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace CoreEngine::Unreal;

void UERenderer::NotePendingExplosions() {
    if (!state_) {
        return;
    }
    for (const auto& exp : state_->GetExplosions()) {
        UE3DExplosionEchoProxy echo;
        echo.positionUE = bridge_.CoreToUEPosition(exp.GetPosition(), 20.0f);
        echo.radiusUE = bridge_.CoreToUEDistance(exp.GetRadius());
        echo.ageSeconds = 0.0f;
        echo.maxLifetimeSeconds = explosionEchoLifetime_;
        echo.color = UEColor::ImpulseOrange();
        explosionEchoes_.push_back(echo);
    }
}

void UERenderer::Render(float renderDeltaTime) {
    lastFrame_ = UE3DRenderFrame{};
    if (!state_) {
        return;
    }

    // Also capture any explosions currently sitting in state_ if NotePendingExplosions wasn't called yet
    if (!state_->GetExplosions().empty()) {
        NotePendingExplosions();
    }

    // Age and prune explosion echoes
    if (renderDeltaTime > 0.0f) {
        for (auto& echo : explosionEchoes_) {
            echo.ageSeconds += renderDeltaTime;
        }
    }
    explosionEchoes_.erase(
        std::remove_if(explosionEchoes_.begin(), explosionEchoes_.end(),
                       [](const UE3DExplosionEchoProxy& e) {
                           return e.ageSeconds >= e.maxLifetimeSeconds;
                       }),
        explosionEchoes_.end());

    lastFrame_.simulationTick = state_->GetSimulationTick();
    const Vector2 mapSize = state_->GetMapSize();
    lastFrame_.arenaMinUE = bridge_.CoreToUEPosition(Vector2(0.0f, 0.0f), 0.0f);
    lastFrame_.arenaMaxUE = bridge_.CoreToUEPosition(mapSize, 0.0f);

    // Arena boundary box primitive
    if (overlays_.showObstaclesAndBounds) {
        UE3DPrimitive arenaBox;
        arenaBox.kind = UE3DPrimitiveKind::Box;
        arenaBox.transform.location = bridge_.CoreToUEPosition(mapSize * 0.5f, 5.0f);
        arenaBox.endPoint = UEVec3(
            bridge_.CoreToUEDistance(mapSize.x * 0.5f),
            bridge_.CoreToUEDistance(mapSize.y * 0.5f),
            10.0f);
        arenaBox.color = UEColor::NeutralGray();
        arenaBox.text = "ArenaBounds";
        lastFrame_.primitives.push_back(arenaBox);
    }

    // Selection box on ground plane
    const SelectionSystem& sel = const_cast<RTSGameState*>(state_)->GetSelectionSystem();
    if (sel.IsBoxSelecting()) {
        lastFrame_.hasSelectionBox = true;
        const Vector2 b0 = sel.GetBoxStart();
        const Vector2 b1 = sel.GetBoxEnd();
        const Vector2 minCore(std::min(b0.x, b1.x), std::min(b0.y, b1.y));
        const Vector2 maxCore(std::max(b0.x, b1.x), std::max(b0.y, b1.y));
        lastFrame_.selectionBoxMinUE = bridge_.CoreToUEPosition(minCore, 15.0f);
        lastFrame_.selectionBoxMaxUE = bridge_.CoreToUEPosition(maxCore, 15.0f);
    }

    // Index diagnostics by entityId for O(1) lookup
    std::unordered_map<uint64_t, const UnitDiagnosticRecord*> diagByEntity;
    if (state_->DiagnosticsEnabled()) {
        for (const auto& rec : state_->GetDiagnostics()) {
            diagByEntity[rec.entityId] = &rec;
        }
    }

    // Territory grid 3D quads
    if (overlays_.showTerritoryGrid && territory_ && territory_->GridCols() > 0 && territory_->GridRows() > 0) {
        const int cols = territory_->GridCols();
        const int rows = territory_->GridRows();
        const float cellW = mapSize.x / static_cast<float>(cols);
        const float cellH = mapSize.y / static_cast<float>(rows);
        const auto& grid = territory_->GetGrid();
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
                UE3DPrimitive quad;
                quad.kind = UE3DPrimitiveKind::Quad3D;
                const Vector2 centerCore((static_cast<float>(c) + 0.5f) * cellW,
                                         (static_cast<float>(r) + 0.5f) * cellH);
                quad.transform.location = bridge_.CoreToUEPosition(centerCore, 2.0f);
                quad.radius = bridge_.CoreToUEDistance(cellW * 0.5f);
                quad.halfHeight = bridge_.CoreToUEDistance(cellH * 0.5f);
                UEColor col = UEColor::ForTeam(cell.owner);
                col.a = (cell.state == TerritoryCalculator::TerritoryCellState::Active) ? 120 : 65;
                quad.color = col;
                lastFrame_.primitives.push_back(quad);
            }
        }
    }

    // Map obstacles and static attractors
    if (overlays_.showObstaclesAndBounds) {
        if (const Map* map = state_->GetMap()) {
            for (const auto& entityPtr : map->GetEntities()) {
                if (!entityPtr) {
                    continue;
                }
                for (const auto& compPtr : entityPtr->GetComponents()) {
                    if (const auto* obsComp = dynamic_cast<const MapObstacleComponent*>(compPtr.get())) {
                        UE3DObstacleProxy obs;
                        obs.positionUE = bridge_.CoreToUEPosition(entityPtr->GetPosition(), 0.0f);
                        obs.radiusUE = bridge_.CoreToUEDistance(obsComp->GetRadius());
                        obs.heightUE = obs.radiusUE * 1.5f;
                        obs.isAttractor = false;
                        obs.strength = obsComp->GetInfluenceFieldItem().GetImportance();
                        obs.color = UEColor::ObstacleSlate();
                        lastFrame_.obstaclesAndAttractors.push_back(obs);

                        UE3DPrimitive cyl;
                        cyl.kind = UE3DPrimitiveKind::Cylinder;
                        cyl.transform.location = obs.positionUE + UEVec3(0.0f, 0.0f, obs.heightUE * 0.5f);
                        cyl.radius = obs.radiusUE;
                        cyl.halfHeight = obs.heightUE * 0.5f;
                        cyl.color = obs.color;
                        cyl.text = "Obstacle";
                        lastFrame_.primitives.push_back(cyl);
                    }
                }
            }
        }
        for (const auto& attr : state_->GetStaticAttractors()) {
            UE3DObstacleProxy proxy;
            proxy.positionUE = bridge_.CoreToUEPosition(attr.GetPosition(), 0.0f);
            proxy.radiusUE = bridge_.CoreToUEDistance(12.0f);
            proxy.heightUE = bridge_.CoreToUEDistance(30.0f);
            proxy.isAttractor = true;
            proxy.strength = attr.GetImportance();
            proxy.color = UEColor::AttractorPurple();
            lastFrame_.obstaclesAndAttractors.push_back(proxy);

            if (overlays_.showInfluenceField) {
                UE3DPrimitive beacon;
                beacon.kind = UE3DPrimitiveKind::Sphere;
                beacon.transform.location = proxy.positionUE + UEVec3(0.0f, 0.0f, proxy.heightUE);
                beacon.radius = proxy.radiusUE;
                beacon.color = proxy.color;
                beacon.text = "StaticAttractor";
                lastFrame_.primitives.push_back(beacon);
            }
        }
    }

    // Units
    for (IGameUnit* unit : state_->GetAllUnits()) {
        if (!unit) {
            continue;
        }
        const uint64_t entityId = state_->GetEntityId(unit);
        const RTSUnit* rtsUnit = dynamic_cast<const RTSUnit*>(unit);

        UE3DUnitRenderProxy proxy;
        proxy.entityId = entityId;
        proxy.team = unit->GetTeam();
        proxy.collisionRadiusUE = bridge_.CoreToUEDistance(unit->GetRadius());
        proxy.visualRadiusUE = rtsUnit
            ? bridge_.CoreToUEDistance(rtsUnit->GetVisualRadius())
            : proxy.collisionRadiusUE;
        if (proxy.visualRadiusUE <= 1.0f) {
            proxy.visualRadiusUE = std::max(10.0f, proxy.collisionRadiusUE * 0.5f);
        }
        proxy.capsuleHalfHeightUE = std::max(proxy.visualRadiusUE * 1.4f, bridge_.CoreToUEDistance(12.0f));

        proxy.isSelected = unit->IsSelected();
        proxy.inDebugMode = unit->IsInDebugMode();
        if (rtsUnit) {
            proxy.currentHP = rtsUnit->GetCurrentHP();
            proxy.maxHP = rtsUnit->GetMaxHP();
            proxy.hpFraction = (proxy.maxHP > 0.0f) ? (proxy.currentHP / proxy.maxHP) : 0.0f;
            proxy.isDead = rtsUnit->IsDead();
            proxy.activeBehaviorState = rtsUnit->GetStateMachine().GetCurrentState();
            proxy.influenceRadiusUE = bridge_.CoreToUEDistance(rtsUnit->GetInfluenceRadius());
            proxy.attackRangeUE = bridge_.CoreToUEDistance(rtsUnit->GetAttackRange());
        }
        proxy.perceptionRadiusUE = bridge_.CoreToUEDistance(state_->GetPerceptionRadius());

        // Heading persistence when velocity is near zero
        float prevYaw = 0.0f;
        auto yawIt = lastUnitHeadingYaw_.find(entityId);
        if (yawIt != lastUnitHeadingYaw_.end()) {
            prevYaw = yawIt->second;
        }
        const float newYaw = bridge_.ComputeHeadingYawDegrees(unit->GetVelocity(), prevYaw);
        lastUnitHeadingYaw_[entityId] = newYaw;

        proxy.bodyTransform = bridge_.BuildUnitTransform(
            unit->GetPosition(), unit->GetVelocity(), proxy.capsuleHalfHeightUE, prevYaw);

        // Nose cone placed on the forward rim of the unit capsule
        const float yawRad = newYaw * CoreEngine::Math::DEG_TO_RAD;
        const UEVec3 forwardDir(std::cos(yawRad), std::sin(yawRad), 0.0f);
        proxy.headingConeTransform = proxy.bodyTransform;
        proxy.headingConeTransform.location =
            proxy.bodyTransform.location + forwardDir * (proxy.visualRadiusUE * 0.9f);

        proxy.bodyColor = proxy.isDead ? UEColor::DeadDark() : UEColor::ForTeam(proxy.team);
        proxy.ringColor = proxy.isSelected ? UEColor::SelectedGold() : proxy.bodyColor;

        // Diagnostics & arrows
        proxy.velocityArrowStartUE = proxy.bodyTransform.location;
        proxy.velocityArrowEndUE =
            proxy.bodyTransform.location + bridge_.CoreToUEVector(unit->GetVelocity() * 0.4f, 0.0f);
        proxy.propulsionArrowEndUE = proxy.bodyTransform.location;
        proxy.impulseArrowEndUE = proxy.bodyTransform.location;

        auto diagIt = diagByEntity.find(entityId);
        if (diagIt != diagByEntity.end() && diagIt->second) {
            const UnitDiagnosticRecord& rec = *diagIt->second;
            if (!rec.behavior.empty() && proxy.activeBehaviorState.empty()) {
                proxy.activeBehaviorState = rec.behavior;
            }
            proxy.transitionedThisTick = rec.transitioned;
            proxy.transitionFrom = rec.fromState;
            proxy.transitionTo = rec.toState;
            proxy.hasTarget = rec.hasTarget;
            if (rec.hasTarget) {
                proxy.targetPositionUE = bridge_.CoreToUEPosition(rec.target, proxy.capsuleHalfHeightUE * 0.5f);
            }
            proxy.propulsionArrowEndUE =
                proxy.bodyTransform.location + bridge_.CoreToUEVector(rec.propulsion * 0.25f, 10.0f);
            proxy.impulseArrowEndUE =
                proxy.bodyTransform.location + bridge_.CoreToUEVector(rec.impulse * 0.35f, 20.0f);
        }

        // Emit 3D primitives for this unit
        UE3DPrimitive bodyCapsule;
        bodyCapsule.kind = UE3DPrimitiveKind::Capsule;
        bodyCapsule.transform = proxy.bodyTransform;
        bodyCapsule.radius = proxy.visualRadiusUE;
        bodyCapsule.halfHeight = proxy.capsuleHalfHeightUE;
        bodyCapsule.color = proxy.bodyColor;
        bodyCapsule.ownerEntityId = entityId;
        lastFrame_.primitives.push_back(bodyCapsule);

        UE3DPrimitive noseCone;
        noseCone.kind = UE3DPrimitiveKind::Cone;
        noseCone.transform = proxy.headingConeTransform;
        noseCone.radius = proxy.visualRadiusUE * 0.45f;
        noseCone.halfHeight = proxy.visualRadiusUE * 0.6f;
        noseCone.color = proxy.ringColor;
        noseCone.ownerEntityId = entityId;
        lastFrame_.primitives.push_back(noseCone);

        if (proxy.isSelected) {
            UE3DPrimitive selRing;
            selRing.kind = UE3DPrimitiveKind::Ring3D;
            selRing.transform.location = bridge_.CoreToUEPosition(unit->GetPosition(), 4.0f);
            selRing.radius = proxy.collisionRadiusUE * 1.3f;
            selRing.color = UEColor::SelectedGold();
            selRing.ownerEntityId = entityId;
            lastFrame_.primitives.push_back(selRing);
        }

        if (overlays_.showVelocityArrows && unit->GetVelocity().LengthSquared() > 1e-3f) {
            UE3DPrimitive velArrow;
            velArrow.kind = UE3DPrimitiveKind::Arrow3D;
            velArrow.transform.location = proxy.velocityArrowStartUE;
            velArrow.endPoint = proxy.velocityArrowEndUE;
            velArrow.color = UEColor::VelocityCyan();
            velArrow.ownerEntityId = entityId;
            velArrow.text = "Velocity";
            lastFrame_.primitives.push_back(velArrow);
        }

        if (overlays_.showPropulsionAndImpulse && diagIt != diagByEntity.end()) {
            const UnitDiagnosticRecord& rec = *diagIt->second;
            if (rec.propulsion.LengthSquared() > 1e-3f) {
                UE3DPrimitive propArrow;
                propArrow.kind = UE3DPrimitiveKind::Arrow3D;
                propArrow.transform.location = proxy.bodyTransform.location;
                propArrow.endPoint = proxy.propulsionArrowEndUE;
                propArrow.color = UEColor::PropulsionGreen();
                propArrow.ownerEntityId = entityId;
                propArrow.text = "Propulsion";
                lastFrame_.primitives.push_back(propArrow);
            }
            if (rec.impulse.LengthSquared() > 1e-3f) {
                UE3DPrimitive impArrow;
                impArrow.kind = UE3DPrimitiveKind::Arrow3D;
                impArrow.transform.location = proxy.bodyTransform.location;
                impArrow.endPoint = proxy.impulseArrowEndUE;
                impArrow.color = UEColor::ImpulseOrange();
                impArrow.ownerEntityId = entityId;
                impArrow.text = "Impulse";
                lastFrame_.primitives.push_back(impArrow);
            }
        }

        if (overlays_.showPerceptionCircles && proxy.perceptionRadiusUE > 0.0f) {
            UE3DPrimitive percRing;
            percRing.kind = UE3DPrimitiveKind::Ring3D;
            percRing.transform.location = bridge_.CoreToUEPosition(unit->GetPosition(), 3.0f);
            percRing.radius = proxy.perceptionRadiusUE;
            percRing.color = UEColor(120, 200, 255, 110);
            percRing.ownerEntityId = entityId;
            lastFrame_.primitives.push_back(percRing);
        }

        if (overlays_.showInfluenceField && proxy.influenceRadiusUE > 0.0f) {
            UE3DPrimitive infSphere;
            infSphere.kind = UE3DPrimitiveKind::WireSphere;
            infSphere.transform.location = proxy.bodyTransform.location;
            infSphere.radius = proxy.influenceRadiusUE;
            infSphere.color = UEColor(180, 130, 255, 100);
            infSphere.ownerEntityId = entityId;
            lastFrame_.primitives.push_back(infSphere);
        }

        if (overlays_.showAttackRanges && proxy.attackRangeUE > 0.0f) {
            UE3DPrimitive atkRing;
            atkRing.kind = UE3DPrimitiveKind::Ring3D;
            atkRing.transform.location = bridge_.CoreToUEPosition(unit->GetPosition(), 5.0f);
            atkRing.radius = proxy.attackRangeUE;
            atkRing.color = UEColor(255, 90, 90, 140);
            atkRing.ownerEntityId = entityId;
            lastFrame_.primitives.push_back(atkRing);
        }

        if (overlays_.showStateBadges) {
            UE3DPrimitive badge;
            badge.kind = UE3DPrimitiveKind::TextBadge3D;
            badge.transform.location =
                proxy.bodyTransform.location + UEVec3(0.0f, 0.0f, proxy.capsuleHalfHeightUE + 35.0f);
            std::ostringstream oss;
            oss << "[#" << entityId << " T" << proxy.team << "] "
                << (proxy.activeBehaviorState.empty() ? "Idle" : proxy.activeBehaviorState)
                << " HP:" << static_cast<int>(std::round(proxy.currentHP))
                << "/" << static_cast<int>(std::round(proxy.maxHP));
            if (proxy.transitionedThisTick) {
                oss << " (" << proxy.transitionFrom << "->" << proxy.transitionTo << ")";
            }
            badge.text = oss.str();
            badge.color = proxy.transitionedThisTick ? UEColor::SelectedGold() : UEColor(240, 245, 255, 255);
            badge.ownerEntityId = entityId;
            lastFrame_.primitives.push_back(badge);
        }

        lastFrame_.units.push_back(std::move(proxy));
    }

    // Active projectiles and explosion echoes
    if (overlays_.showProjectilesAndExplosions) {
        for (const auto& proj : state_->GetProjectiles().Projectiles()) {
            UE3DProjectileProxy pProxy;
            pProxy.projectileId = proj.id;
            pProxy.ownerId = proj.ownerId;
            pProxy.team = proj.team;
            pProxy.positionUE = bridge_.CoreToUEPosition(proj.position, 45.0f);
            pProxy.velocityUE = bridge_.CoreToUEVector(proj.velocity, 0.0f);
            pProxy.radiusUE = std::max(15.0f, bridge_.CoreToUEDistance(proj.radius));
            pProxy.color = UEColor::ForTeam(proj.team);
            lastFrame_.projectiles.push_back(pProxy);

            UE3DPrimitive sphere;
            sphere.kind = UE3DPrimitiveKind::Sphere;
            sphere.transform.location = pProxy.positionUE;
            sphere.radius = pProxy.radiusUE;
            sphere.color = pProxy.color;
            sphere.ownerEntityId = proj.ownerId;
            sphere.text = "Projectile";
            lastFrame_.primitives.push_back(sphere);
        }

        for (const auto& echo : explosionEchoes_) {
            lastFrame_.explosions.push_back(echo);
            UE3DPrimitive shockwave;
            shockwave.kind = UE3DPrimitiveKind::WireSphere;
            shockwave.transform.location = echo.positionUE;
            const float progress = (echo.maxLifetimeSeconds > 0.0f)
                ? std::min(1.0f, echo.ageSeconds / echo.maxLifetimeSeconds)
                : 1.0f;
            shockwave.radius = echo.radiusUE * (0.4f + 0.6f * progress);
            shockwave.color = echo.color;
            shockwave.text = "Explosion";
            lastFrame_.primitives.push_back(shockwave);
        }
    }
}
