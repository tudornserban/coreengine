#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "CoreEngineCoreBridge.h"
#include "CoreEngineUE5Actors.h"
#include "CoreEngineVisualTestActor.generated.h"

UENUM(BlueprintType)
enum class ECoreEngineSandboxPreset : uint8 {
    SingleUnitBehaviorLab = 0 UMETA(DisplayName = "Single-Unit Behavior Lab"),
    SingleUnitStateMachineLab = 1 UMETA(DisplayName = "Single-Unit State Machine Lab"),
    MultiUnitSquadAndSwarm = 2 UMETA(DisplayName = "Multi-Unit Squad & Swarm"),
    CombatProjectilesAndExplosions = 3 UMETA(DisplayName = "Combat, Projectiles & Explosions"),
    DecisionGraphAndTurnClashLab = 4 UMETA(DisplayName = "Decision Graph & Turn Clash Lab")
};

/// Placeable UE5 Actor that runs the CoreEngine 3D Placeholder Sandbox, handles 3D mouse/keyboard
/// input, draws all 3D placeholder primitives (capsules, nose cones, velocity/force arrows,
/// influence spheres, projectiles, explosions, territory grid, floating FSM badges), and exposes
/// one-click CallInEditor buttons in the UE5 Details panel.
UCLASS(BlueprintType, Blueprintable)
class COREENGINEUE5_API ACoreEngineVisualTestActor : public AActor {
    GENERATED_BODY()

public:
    ACoreEngineVisualTestActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|Setup")
    ECoreEngineSandboxPreset InitialPreset = ECoreEngineSandboxPreset::SingleUnitBehaviorLab;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|Setup", meta = (ClampMin = "1", ClampMax = "32"))
    int32 UnitsPerTeam = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|Setup", meta = (ClampMin = "0.1", ClampMax = "100.0"))
    float WorldScale = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|Setup")
    bool bAutoPositionCameraOnBeginPlay = true;

    // --- One-Click UE5 Editor & Blueprint Actions ---
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|1_Presets")
    void LoadSingleUnitBehaviorLab();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|1_Presets")
    void LoadSingleUnitStateMachineLab();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|1_Presets")
    void LoadMultiUnitSwarmLab();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|1_Presets")
    void LoadCombatLab();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|1_Presets")
    void LoadDecisionGraphAndClashLab();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|2_Playback")
    void TogglePause();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|2_Playback")
    void StepSingleTick();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|2_Playback")
    void ResetScenario();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToSeek();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToFlee();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToWander();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToCohesion();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToSeparation();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void SwapBehaviorToTacticalFrontline();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void EjectBehavior();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void ApplyFSMPatrolEngageFlee();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void ApplyFSMSwarmToFrontline();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|3_BehaviorsAndFSM")
    void EvaluateDecisionGraph();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|4_UnitsCharacterInventory")
    void SpawnTeam1Unit();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|4_UnitsCharacterInventory")
    void SpawnTeam2Unit();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|4_UnitsCharacterInventory")
    void EquipWeaponAndArmorOnFocusUnit();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|4_UnitsCharacterInventory")
    void UseRepairKitOnFocusUnit();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|5_CombatAndMinimap")
    void FireTestProjectile();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|5_CombatAndMinimap")
    void DetonateTestExplosion();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|5_CombatAndMinimap")
    void TriggerTurnClash();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "CoreEngine|5_CombatAndMinimap")
    void SendMinimapPing();

    CoreEngine::Unreal::FCoreEngineRuntimeBridge* GetBridge() { return RuntimeBridge.get(); }
    const CoreEngine::Unreal::FCoreEngineRuntimeBridge* GetBridge() const { return RuntimeBridge.get(); }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void EnsureControllerInitialized();
    void PollPlayerControllerInput();

    std::unique_ptr<CoreEngine::Unreal::FCoreEngineRuntimeBridge> RuntimeBridge;
};

/// Zero-asset UCanvas HUD that renders all 7 UEDefaultGUI panels, interactive clickable buttons,
/// and the 2D Tactical Minimap & Radar directly on screen during Play-In-Editor.
UCLASS(BlueprintType, Blueprintable)
class COREENGINEUE5_API ACoreEngineVisualTestHUD : public AHUD {
    GENERATED_BODY()

public:
    ACoreEngineVisualTestHUD();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|HUD")
    bool bShowDefaultGUIPanels = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreEngine|HUD")
    bool bShowMinimap = true;

    virtual void DrawHUD() override;

private:
    struct FClickableButtonRect {
        FVector2D Min;
        FVector2D Max;
        std::string WidgetId;
    };

    ACoreEngineVisualTestActor* FindVisualTestActor() const;
    void DrawGuiPanels(CoreEngine::Unreal::FCoreEngineRuntimeBridge& Bridge);
    void DrawMinimapWidget(CoreEngine::Unreal::FCoreEngineRuntimeBridge& Bridge);
    void HandleHudMouseClicks(CoreEngine::Unreal::FCoreEngineRuntimeBridge& Bridge);

    std::vector<FClickableButtonRect> ButtonRectsThisFrame;
    FVector2D MinimapOriginPx{980.0f, 440.0f};
    FVector2D MinimapSizePx{260.0f, 240.0f};
    bool bWasLeftMouseDown = false;
    bool bWasRightMouseDown = false;
    bool bHudConsumedClickThisFrame = false;

public:
    bool DidHudConsumeClick() const { return bHudConsumedClickThisFrame; }
};

/// Turnkey GameMode that sets ACoreEngineVisualTestHUD as the active HUD and automatically
/// spawns ACoreEngineVisualTestActor when Play is pressed in any level.
UCLASS(BlueprintType, Blueprintable)
class COREENGINEUE5_API ACoreEngineVisualTestGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    ACoreEngineVisualTestGameMode();
    virtual void BeginPlay() override;
};
