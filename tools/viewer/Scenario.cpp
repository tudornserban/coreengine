#include "Scenario.h"

#include "core/gamestate/RTSGameState.h"
#include "core/gamestate/RTSUnit.h"
#include "core/math/Vector2.h"

Scene BuildPreset(Preset preset, std::uint64_t seed)
{
    Scene scene;
    switch (preset)
    {
    case Preset::TwoUnits:
        scene.state = std::make_unique<RTSGameState>(Vector2(800.0f, 600.0f), seed);
        scene.state->AddUnit(std::make_unique<RTSUnit>(Vector2(200.0f, 300.0f), 1, 12.0f, 100.0f));
        scene.state->AddUnit(std::make_unique<RTSUnit>(Vector2(600.0f, 300.0f), 2, 12.0f, 100.0f));
        break;
    }
    return scene;
}
