#pragma once

#include <cstdint>
#include <memory>

class RTSGameState;

enum class Preset
{
    TwoUnits
};

struct Scene
{
    std::unique_ptr<RTSGameState> state;
};

Scene BuildPreset(Preset preset, std::uint64_t seed);
