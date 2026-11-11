#pragma once

#include "core/combat/Explosion.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class RTSGameState;

enum class Preset
{
    TwoUnits,
    Collision,
    Combat,
    Ai
};

struct Scene
{
    std::unique_ptr<RTSGameState> state;
};

struct ScenarioEvent
{
    float time = 0.0f;
    Explosion explosion{Vector2::Zero(), 0.0f, 0.0f};
    bool fired = false;
};

Scene BuildPreset(Preset preset, std::uint64_t seed);
std::vector<ScenarioEvent> PendingEvents(Preset preset, const RTSGameState& state);

const char* PresetToCliName(Preset preset);
bool ParsePresetFromString(const std::string& name, Preset& out);
