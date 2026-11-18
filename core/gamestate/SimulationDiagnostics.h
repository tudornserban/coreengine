#pragma once

#include "core/math/Vector2.h"

#include <cstdint>
#include <map>
#include <string>

/// Immutable diagnostic captured during a simulation tick. Safe after the unit is removed.
struct UnitDiagnosticRecord {
    uint64_t entityId = 0;
    uint32_t tick = 0;
    std::string behavior;
    bool transitioned = false;
    std::string fromState;
    std::string toState;
    bool hasTarget = false;
    Vector2 target;
    Vector2 propulsion;
    Vector2 impulse;
    std::map<std::string, float> extensions;
};

enum class LifecycleEventKind : uint8_t {
    Died = 1
};

struct LifecycleEvent {
    LifecycleEventKind kind = LifecycleEventKind::Died;
    uint64_t entityId = 0;
    uint32_t tick = 0;
};
