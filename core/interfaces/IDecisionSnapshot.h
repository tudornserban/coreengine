#pragma once

#include <cstdint>
#include <string>

namespace CoreEngine {

enum class DecisionTarget : uint8_t { Self = 0, Enemy = 1, World = 2 };

/// Read-only property query contract for evaluating decision graphs and AI conditions.
/// Implemented by game-layer combat snapshots or RTS unit adapters.
class IDecisionSnapshot {
public:
    virtual ~IDecisionSnapshot() = default;

    /// Float attribute (e.g. "hp", "hp_pct", "stamina_pct", "adrenaline", "telegraph_confidence").
    virtual float GetFloatProperty(DecisionTarget target, const std::string& key,
                                   float defaultValue = 0.0f) const = 0;

    /// Integer/enum attribute (e.g. "momentum", "turn_number", "last_move", "repeated_move_count",
    /// "telegraphed_move").
    virtual int GetIntProperty(DecisionTarget target, const std::string& key, int defaultValue = 0) const = 0;

    /// Boolean flag (e.g. "has_initiative", "is_exhausted").
    virtual bool GetBoolProperty(DecisionTarget target, const std::string& key,
                                 bool defaultValue = false) const = 0;
};

} // namespace CoreEngine