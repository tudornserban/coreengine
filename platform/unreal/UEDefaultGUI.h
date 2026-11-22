#pragma once

#include "platform/unreal/UEPlaceholder3DSandbox.h"

#include <cstdint>
#include <string>
#include <vector>

namespace CoreEngine {
namespace Unreal {

enum class UEGuiPanelId : uint8_t {
    PlaybackAndMatch = 0,
    BehaviorAndStateMachine = 1,
    UnitCharacterInventory = 2,
    CombatAndClash = 3,
    InfluenceMapCollision = 4,
    SerializationAndSession = 5,
    Minimap = 6
};

enum class UEGuiWidgetKind : uint8_t {
    Label = 0,
    Button = 1,
    ToggleButton = 2,
    Slider = 3,
    ProgressBar = 4,
    KeyValueRow = 5,
    SectionHeader = 6
};

struct UEGuiWidgetModel {
    std::string widgetId;
    UEGuiWidgetKind kind = UEGuiWidgetKind::Label;
    std::string label;
    std::string valueText;
    float numericValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool isChecked = false;
    bool isEnabled = true;
    UEColor accentColor = UEColor(220, 225, 235, 255);
};

struct UEGuiPanelModel {
    UEGuiPanelId panelId = UEGuiPanelId::PlaybackAndMatch;
    std::string title;
    bool visible = true;
    float screenX = 16.0f;
    float screenY = 16.0f;
    float width = 340.0f;
    float height = 280.0f;
    std::vector<UEGuiWidgetModel> widgets;
};

/// Complete frame-by-frame GUI view-model for rendering via UE5 AHUD/UCanvas, Slate, or UMG.
struct UEGuiFrameModel {
    uint32_t simulationTick = 0;
    std::string activePresetName;
    std::string statusBanner;
    std::vector<UEGuiPanelModel> panels;
    UEMinimapFrame minimapFrame;
};

/// Default GUI controller & view-model generator covering all CoreEngine functionality:
/// GameLoop/Playback, AI Behaviors & StateMachines, DecisionGraph, Units/Character/Inventory,
/// TimedActions, Combat/Projectiles/Explosions/TurnClash, Influence/Territory/Tactical Checkers,
/// Map/Placement/ResourceClaims, Canonical Snapshots/JSON Serialization, Sessions, and Minimap.
class UEDefaultGUI {
private:
    UEPlaceholder3DSandbox* sandbox_ = nullptr;
    std::map<UEGuiPanelId, bool> panelVisibility_;
    std::string lastStatusBanner_ = "CoreEngine UE5 Default GUI Ready";
    UEGuiFrameModel lastGuiFrame_;

public:
    explicit UEDefaultGUI(UEPlaceholder3DSandbox* sandbox = nullptr);

    void SetSandbox(UEPlaceholder3DSandbox* sandbox) { sandbox_ = sandbox; }
    UEPlaceholder3DSandbox* GetSandbox() { return sandbox_; }
    const UEPlaceholder3DSandbox* GetSandbox() const { return sandbox_; }

    void SetPanelVisible(UEGuiPanelId panelId, bool visible) { panelVisibility_[panelId] = visible; }
    bool IsPanelVisible(UEGuiPanelId panelId) const;
    void TogglePanelVisible(UEGuiPanelId panelId) {
        panelVisibility_[panelId] = !IsPanelVisible(panelId);
    }

    /// Build the complete GUI view-model from the current sandbox and CoreEngine state.
    const UEGuiFrameModel& BuildFrameModel();
    const UEGuiFrameModel& GetLastFrameModel() const { return lastGuiFrame_; }

    /// Execute a button or toggle action by widgetId (e.g. "playback.toggle_pause",
    /// "ai.set_behavior:Seek", "fsm.preset:PatrolEngageFlee", "inv.use:consumable_nanite_repair").
    bool ExecuteButtonAction(const std::string& widgetId);

    /// Apply a slider value change by widgetId (e.g. "ai.param:weight", "unit.max_speed", "session.rtt_ms").
    bool ApplySliderValue(const std::string& widgetId, float value);

    /// Handle keyboard shortcuts from UEInputReader (Space=pause, .=step, 1..4=speed, R=reset, F1..F6=overlays).
    void ProcessKeyboardShortcuts(UEInputReader& input);

    const std::string& GetLastStatusBanner() const { return lastStatusBanner_; }

private:
    UEGuiPanelModel BuildPlaybackAndMatchPanel() const;
    UEGuiPanelModel BuildBehaviorAndStateMachinePanel() const;
    UEGuiPanelModel BuildUnitCharacterInventoryPanel() const;
    UEGuiPanelModel BuildCombatAndClashPanel() const;
    UEGuiPanelModel BuildInfluenceMapCollisionPanel() const;
    UEGuiPanelModel BuildSerializationAndSessionPanel() const;
    UEGuiPanelModel BuildMinimapPanel() const;
};

} // namespace Unreal
} // namespace CoreEngine
