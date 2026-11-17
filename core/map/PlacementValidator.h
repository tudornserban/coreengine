#pragma once

#include "core/interfaces/IGameUnit.h"
#include "core/map/Obstacle.h"

#include <functional>
#include <vector>

/// Stateless spawn check for arena bounds, placement obstacles, team spacing,
/// and an optional territory predicate supplied by the game layer.
class PlacementValidator {
public:
    struct Rules {
        /// Same-team center clearance. Enemy clearance uses this value unless
        /// `enemyClearanceUsesInfluence` is set. Zero disables that term.
        float baseSeparationRadius = 0.0f;
        /// Extra center clearance applied against every living unit. The
        /// required distance is the max of this and the team/enemy clearance.
        float minPlacementDistance = 0.0f;
        bool enemyClearanceUsesInfluence = false;
        bool wrapBorders = false;
        /// Return false to reject the (possibly wrapped) point. Empty allows all.
        std::function<bool(float x, float y, int teamId)> territoryFilter;
    };

    /// `units` may mix teams. Dead `IUnitCombat` units are ignored.
    /// Points outside a non-wrapping arena are rejected. Wrapping arenas fold
    /// the candidate into `[0, arenaW)` x `[0, arenaH)` before the other checks.
    [[nodiscard]] bool IsValidPlacement(
        float x,
        float y,
        int placingTeamId,
        const std::vector<const IGameUnit*>& units,
        const std::vector<Obstacle>& obstacles,
        float arenaW,
        float arenaH,
        const Rules& rules) const;
};
