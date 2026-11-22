#include "CoreEngineVisualTestActor.h"

#if defined(COREENGINE_WITH_UE5)
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

using namespace CoreEngine::Unreal;

ACoreEngineVisualTestActor::ACoreEngineVisualTestActor() {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void ACoreEngineVisualTestActor::EnsureControllerInitialized() {
    if (!SubsystemController) {
        UECoordinateConfig Cfg;
        Cfg.worldScale = FMath::Max(0.1f, WorldScale);
        const FVector ActorLoc = GetActorLocation();
        Cfg.originOffset = UEVec3(ActorLoc.X, ActorLoc.Y, ActorLoc.Z);
        SubsystemController = std::make_unique<UEWorldSubsystemController>(
            Vector2(800.0f, 600.0f), 42, Cfg);
        SubsystemController->Initialize(
            static_cast<UESandboxPreset>(InitialPreset), FMath::Max(1, UnitsPerTeam));
    }
}

void ACoreEngineVisualTestActor::BeginPlay() {
    Super::BeginPlay();
    EnsureControllerInitialized();

    if (UWorld* World = GetWorld()) {
        if (APlayerController* PC = World->GetFirstPlayerController()) {
            PC->bShowMouseCursor = true;
            PC->bEnableClickEvents = true;
            PC->bEnableMouseOverEvents = true;

            if (bAutoPositionCameraOnBeginPlay && PC->GetPawn()) {
                const UEVec3 CenterUE = SubsystemController->GetSandbox().GetCoordinateBridge().CoreToUEPosition(
                    Vector2(400.0f, 300.0f), 4200.0f);
                PC->GetPawn()->SetActorLocation(FVector(CenterUE.x - 1800.0f, CenterUE.y, CenterUE.z));
                PC->SetControlRotation(FRotator(-62.0f, 0.0f, 0.0f));
            }
        }
    }
}

void ACoreEngineVisualTestActor::PollPlayerControllerInput() {
    if (!SubsystemController) {
        return;
    }
    UWorld* World = GetWorld();
    if (!World) {
        return;
    }
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) {
        return;
    }

    auto& Input = SubsystemController->GetSandbox().GetInputReader();
    const auto& Bridge = SubsystemController->GetSandbox().GetCoordinateBridge();

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (PC->GetMousePosition(MouseX, MouseY)) {
        Input.InjectScreenMousePosition(Vector2(MouseX, MouseY));
    }

    FVector WorldOrigin, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection)) {
        Input.InjectWorldCameraRay(
            UEVec3(WorldOrigin.X, WorldOrigin.Y, WorldOrigin.Z),
            UEVec3(WorldDirection.X, WorldDirection.Y, WorldDirection.Z),
            Bridge);
    }

    bool bHudConsumed = false;
    if (const auto* TestHud = Cast<ACoreEngineVisualTestHUD>(PC->GetHUD())) {
        bHudConsumed = TestHud->DidHudConsumeClick();
    }

    if (!bHudConsumed) {
        Input.InjectMouseButtonState(UE_MOUSE_LEFT, PC->IsInputKeyDown(EKeys::LeftMouseButton));
        Input.InjectMouseButtonState(UE_MOUSE_RIGHT, PC->IsInputKeyDown(EKeys::RightMouseButton));
    } else {
        Input.InjectMouseButtonState(UE_MOUSE_LEFT, false);
        Input.InjectMouseButtonState(UE_MOUSE_RIGHT, false);
    }

    Input.InjectKeyState(UE_KEY_LSHIFT, PC->IsInputKeyDown(EKeys::LeftShift));
    Input.InjectKeyState(UE_KEY_SPACE, PC->IsInputKeyDown(EKeys::SpaceBar));
    Input.InjectKeyState(UE_KEY_PERIOD, PC->IsInputKeyDown(EKeys::Period));
    Input.InjectKeyState(UE_KEY_1, PC->IsInputKeyDown(EKeys::One));
    Input.InjectKeyState(UE_KEY_2, PC->IsInputKeyDown(EKeys::Two));
    Input.InjectKeyState(UE_KEY_3, PC->IsInputKeyDown(EKeys::Three));
    Input.InjectKeyState(UE_KEY_4, PC->IsInputKeyDown(EKeys::Four));
    Input.InjectKeyState(UE_KEY_R, PC->IsInputKeyDown(EKeys::R));
    Input.InjectKeyState(UE_KEY_F1, PC->IsInputKeyDown(EKeys::F1));
    Input.InjectKeyState(UE_KEY_F2, PC->IsInputKeyDown(EKeys::F2));
    Input.InjectKeyState(UE_KEY_F3, PC->IsInputKeyDown(EKeys::F3));
    Input.InjectKeyState(UE_KEY_F4, PC->IsInputKeyDown(EKeys::F4));
    Input.InjectKeyState(UE_KEY_F5, PC->IsInputKeyDown(EKeys::F5));
    Input.InjectKeyState(UE_KEY_F6, PC->IsInputKeyDown(EKeys::F6));
}

void ACoreEngineVisualTestActor::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);
    EnsureControllerInitialized();

    PollPlayerControllerInput();
    SubsystemController->Tick(DeltaSeconds);

    const UE3DRenderFrame& Frame3D = SubsystemController->GetSandbox().GetRenderer().GetLastRenderFrame();
    DrawUE5Placeholder3DFrame(GetWorld(), Frame3D, 0.0f);
}

void ACoreEngineVisualTestActor::LoadSingleUnitBehaviorLab() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("preset.single_behavior");
}

void ACoreEngineVisualTestActor::LoadSingleUnitStateMachineLab() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("preset.single_fsm");
}

void ACoreEngineVisualTestActor::LoadMultiUnitSwarmLab() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("preset.multi_swarm");
}

void ACoreEngineVisualTestActor::LoadCombatLab() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("preset.combat");
}

void ACoreEngineVisualTestActor::LoadDecisionGraphAndClashLab() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("preset.decision_clash");
}

void ACoreEngineVisualTestActor::TogglePause() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("playback.toggle_pause");
}

void ACoreEngineVisualTestActor::StepSingleTick() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("playback.step_tick");
}

void ACoreEngineVisualTestActor::ResetScenario() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("sandbox.reset");
}

void ACoreEngineVisualTestActor::SwapBehaviorToSeek() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:Seek");
}

void ACoreEngineVisualTestActor::SwapBehaviorToFlee() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:Flee");
}

void ACoreEngineVisualTestActor::SwapBehaviorToWander() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:Wander");
}

void ACoreEngineVisualTestActor::SwapBehaviorToCohesion() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:Cohesion");
}

void ACoreEngineVisualTestActor::SwapBehaviorToSeparation() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:Separation");
}

void ACoreEngineVisualTestActor::SwapBehaviorToTacticalFrontline() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.set_behavior:TacticalFrontline");
}

void ACoreEngineVisualTestActor::EjectBehavior() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.eject_state");
}

void ACoreEngineVisualTestActor::ApplyFSMPatrolEngageFlee() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("fsm.preset:PatrolEngageFlee");
}

void ACoreEngineVisualTestActor::ApplyFSMSwarmToFrontline() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("fsm.preset:SwarmToFrontline");
}

void ACoreEngineVisualTestActor::EvaluateDecisionGraph() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("ai.eval_decision_graph");
}

void ACoreEngineVisualTestActor::SpawnTeam1Unit() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("unit.spawn_team1");
}

void ACoreEngineVisualTestActor::SpawnTeam2Unit() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("unit.spawn_team2");
}

void ACoreEngineVisualTestActor::EquipWeaponAndArmorOnFocusUnit() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("inv.equip:weapon_plasma_blade");
    SubsystemController->GetGUI().ExecuteButtonAction("inv.equip:armor_aegis_plate");
}

void ACoreEngineVisualTestActor::UseRepairKitOnFocusUnit() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("inv.use:consumable_nanite_repair");
}

void ACoreEngineVisualTestActor::FireTestProjectile() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("combat.fire_projectile");
}

void ACoreEngineVisualTestActor::DetonateTestExplosion() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("combat.trigger_explosion");
}

void ACoreEngineVisualTestActor::TriggerTurnClash() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("combat.exec_turn_clash");
}

void ACoreEngineVisualTestActor::SendMinimapPing() {
    EnsureControllerInitialized();
    SubsystemController->GetGUI().ExecuteButtonAction("minimap.add_ping");
}

// ============================================================================
// ACoreEngineVisualTestHUD
// ============================================================================

ACoreEngineVisualTestHUD::ACoreEngineVisualTestHUD() {}

ACoreEngineVisualTestActor* ACoreEngineVisualTestHUD::FindVisualTestActor() const {
    if (!GetWorld()) {
        return nullptr;
    }
    for (TActorIterator<ACoreEngineVisualTestActor> It(GetWorld()); It; ++It) {
        return *It;
    }
    return nullptr;
}

void ACoreEngineVisualTestHUD::DrawHUD() {
    Super::DrawHUD();
    if (!Canvas) {
        return;
    }
    ACoreEngineVisualTestActor* TestActor = FindVisualTestActor();
    if (!TestActor || !TestActor->GetController()) {
        return;
    }
    UEWorldSubsystemController& Ctrl = *TestActor->GetController();

    ButtonRectsThisFrame.clear();
    if (bShowDefaultGUIPanels) {
        DrawGuiPanels(Ctrl);
    }
    if (bShowMinimap) {
        DrawMinimapWidget(Ctrl);
    }
    HandleHudMouseClicks(Ctrl);
}

void ACoreEngineVisualTestHUD::DrawGuiPanels(UEWorldSubsystemController& Ctrl) {
    const UEGuiFrameModel& GuiModel = Ctrl.GetGUI().GetLastFrameModel();
    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;

    // Top status banner
    DrawRect(FLinearColor(0.05f, 0.07f, 0.11f, 0.88f), 12.0f, 6.0f, 760.0f, 24.0f);
    FString BannerText = FString::Printf(
        TEXT("CoreEngine UE5 | Preset: %s | Tick: %u | %s  [Space:Pause  .:Step  1-4:Speed  R:Reset  F1-F6:3D Overlays]"),
        UTF8_TO_TCHAR(GuiModel.activePresetName.c_str()),
        GuiModel.simulationTick,
        UTF8_TO_TCHAR(GuiModel.statusBanner.c_str()));
    DrawText(BannerText, FLinearColor(0.95f, 0.92f, 0.55f, 1.0f), 18.0f, 10.0f, Font, 1.0f);

    // Compact 3-column layout of the panels
    float ColX[3] = { 12.0f, 340.0f, 668.0f };
    float ColY[3] = { 36.0f, 36.0f, 36.0f };
    const float PanelW = 320.0f;
    const float RowH = 18.0f;

    for (std::size_t pIdx = 0; pIdx < GuiModel.panels.size(); ++pIdx) {
        const UEGuiPanelModel& Panel = GuiModel.panels[pIdx];
        if (!Panel.visible || Panel.panelId == UEGuiPanelId::Minimap) {
            continue;
        }
        const int Col = static_cast<int>(pIdx % 3);
        const float StartX = ColX[Col];
        float CurY = ColY[Col];

        const float EstimatedH = 24.0f + static_cast<float>(Panel.widgets.size()) * RowH;
        DrawRect(FLinearColor(0.06f, 0.08f, 0.12f, 0.82f), StartX, CurY, PanelW, EstimatedH);
        DrawRect(FLinearColor(0.14f, 0.24f, 0.42f, 0.95f), StartX, CurY, PanelW, 20.0f);
        DrawText(UTF8_TO_TCHAR(Panel.title.c_str()), FLinearColor::White, StartX + 6.0f, CurY + 2.0f, Font, 1.0f);
        CurY += 22.0f;

        for (const UEGuiWidgetModel& W : Panel.widgets) {
            switch (W.kind) {
                case UEGuiWidgetKind::SectionHeader: {
                    DrawText(UTF8_TO_TCHAR(W.label.c_str()),
                             FLinearColor(0.55f, 0.78f, 1.0f, 1.0f),
                             StartX + 6.0f, CurY + 1.0f, Font, 1.0f);
                    break;
                }
                case UEGuiWidgetKind::KeyValueRow:
                case UEGuiWidgetKind::Slider: {
                    FString Line = FString::Printf(TEXT("%s: %s"),
                        UTF8_TO_TCHAR(W.label.c_str()),
                        UTF8_TO_TCHAR(W.valueText.c_str()));
                    DrawText(Line, FLinearColor(0.86f, 0.88f, 0.92f, 1.0f), StartX + 10.0f, CurY + 1.0f, Font, 1.0f);
                    break;
                }
                case UEGuiWidgetKind::ProgressBar: {
                    DrawRect(FLinearColor(0.15f, 0.16f, 0.20f, 0.9f), StartX + 8.0f, CurY + 1.0f, PanelW - 16.0f, RowH - 3.0f);
                    const float FillW = (PanelW - 16.0f) * FMath::Clamp(W.numericValue, 0.0f, 1.0f);
                    const FLinearColor BarCol(W.accentColor.r / 255.0f, W.accentColor.g / 255.0f, W.accentColor.b / 255.0f, 0.85f);
                    DrawRect(BarCol, StartX + 8.0f, CurY + 1.0f, FillW, RowH - 3.0f);
                    FString Line = FString::Printf(TEXT("%s (%s)"),
                        UTF8_TO_TCHAR(W.label.c_str()),
                        UTF8_TO_TCHAR(W.valueText.c_str()));
                    DrawText(Line, FLinearColor::White, StartX + 12.0f, CurY + 1.0f, Font, 1.0f);
                    break;
                }
                case UEGuiWidgetKind::Button:
                case UEGuiWidgetKind::ToggleButton: {
                    const FLinearColor BtnBg = W.isChecked
                        ? FLinearColor(0.18f, 0.46f, 0.26f, 0.92f)
                        : FLinearColor(0.18f, 0.22f, 0.30f, 0.92f);
                    DrawRect(BtnBg, StartX + 8.0f, CurY + 1.0f, PanelW - 16.0f, RowH - 2.0f);
                    FString BtnLabel = (W.kind == UEGuiWidgetKind::ToggleButton)
                        ? FString::Printf(TEXT("[%s] %s"), W.isChecked ? TEXT("ON") : TEXT("OFF"), UTF8_TO_TCHAR(W.label.c_str()))
                        : FString::Printf(TEXT("> %s"), UTF8_TO_TCHAR(W.label.c_str()));
                    DrawText(BtnLabel, FLinearColor::White, StartX + 12.0f, CurY + 1.0f, Font, 1.0f);

                    FClickableButtonRect Rect;
                    Rect.Min = FVector2D(StartX + 8.0f, CurY + 1.0f);
                    Rect.Max = FVector2D(StartX + PanelW - 8.0f, CurY + RowH - 1.0f);
                    Rect.WidgetId = W.widgetId;
                    ButtonRectsThisFrame.push_back(std::move(Rect));
                    break;
                }
                default:
                    break;
            }
            CurY += RowH;
        }
        ColY[Col] = CurY + 8.0f;
    }
}

void ACoreEngineVisualTestHUD::DrawMinimapWidget(UEWorldSubsystemController& Ctrl) {
    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
    auto& Minimap = Ctrl.GetSandbox().GetMinimapSystem();
    const float CanvasW = Canvas->SizeX;
    const float CanvasH = Canvas->SizeY;

    MinimapSizePx = FVector2D(260.0f, 220.0f);
    MinimapOriginPx = FVector2D(FMath::Max(16.0f, CanvasW - MinimapSizePx.X - 18.0f),
                                FMath::Max(36.0f, CanvasH - MinimapSizePx.Y - 18.0f));
    Minimap.GetConfig().screenOriginX = MinimapOriginPx.X;
    Minimap.GetConfig().screenOriginY = MinimapOriginPx.Y;
    Minimap.GetConfig().screenWidthPx = MinimapSizePx.X;
    Minimap.GetConfig().screenHeightPx = MinimapSizePx.Y;

    const UEMinimapFrame& Frame = Minimap.GetLastFrame();

    // Header + radar background
    DrawRect(FLinearColor(0.12f, 0.20f, 0.35f, 0.95f), MinimapOriginPx.X, MinimapOriginPx.Y - 20.0f, MinimapSizePx.X, 20.0f);
    FString Title = FString::Printf(TEXT("Minimap (T1: %.0f%% | T2: %.0f%%)"),
                                    Frame.team1TerritoryPct, Frame.team2TerritoryPct);
    DrawText(Title, FLinearColor::White, MinimapOriginPx.X + 6.0f, MinimapOriginPx.Y - 18.0f, Font, 1.0f);
    DrawRect(FLinearColor(0.03f, 0.05f, 0.08f, 0.90f), MinimapOriginPx.X, MinimapOriginPx.Y, MinimapSizePx.X, MinimapSizePx.Y);

    // Territory heatmap cells
    for (const auto& Cell : Frame.territoryCells) {
        const float X0 = MinimapOriginPx.X + Cell.minUV.x * MinimapSizePx.X;
        const float Y0 = MinimapOriginPx.Y + Cell.minUV.y * MinimapSizePx.Y;
        const float W  = (Cell.maxUV.x - Cell.minUV.x) * MinimapSizePx.X;
        const float H  = (Cell.maxUV.y - Cell.minUV.y) * MinimapSizePx.Y;
        const FLinearColor Col(Cell.color.r / 255.0f, Cell.color.g / 255.0f, Cell.color.b / 255.0f, 0.32f);
        DrawRect(Col, X0, Y0, W, H);
    }

    // Obstacles & resources
    for (const auto& Obs : Frame.obstacleBlips) {
        const float X = MinimapOriginPx.X + Obs.uv.x * MinimapSizePx.X;
        const float Y = MinimapOriginPx.Y + Obs.uv.y * MinimapSizePx.Y;
        DrawRect(FLinearColor(0.45f, 0.48f, 0.54f, 0.95f), X - 4.0f, Y - 4.0f, 8.0f, 8.0f);
    }
    for (const auto& Res : Frame.resourceBlips) {
        const float X = MinimapOriginPx.X + Res.uv.x * MinimapSizePx.X;
        const float Y = MinimapOriginPx.Y + Res.uv.y * MinimapSizePx.Y;
        DrawRect(FLinearColor(0.95f, 0.80f, 0.25f, 0.95f), X - 4.0f, Y - 4.0f, 8.0f, 8.0f);
    }

    // Units
    for (const auto& UnitBlip : Frame.unitBlips) {
        const float X = MinimapOriginPx.X + UnitBlip.uv.x * MinimapSizePx.X;
        const float Y = MinimapOriginPx.Y + UnitBlip.uv.y * MinimapSizePx.Y;
        const float Sz = UnitBlip.isHeroCharacter ? 8.0f : 6.0f;
        const FLinearColor Col(UnitBlip.color.r / 255.0f, UnitBlip.color.g / 255.0f, UnitBlip.color.b / 255.0f, 1.0f);
        DrawRect(Col, X - Sz * 0.5f, Y - Sz * 0.5f, Sz, Sz);
    }

    // Radar pings
    for (const auto& Ping : Frame.activePings) {
        const float X = MinimapOriginPx.X + Ping.uv.x * MinimapSizePx.X;
        const float Y = MinimapOriginPx.Y + Ping.uv.y * MinimapSizePx.Y;
        const float R = 6.0f + Ping.PulseFraction() * 14.0f;
        DrawLine(X - R, Y, X + R, Y, FLinearColor::Yellow, 1.5f);
        DrawLine(X, Y - R, X, Y + R, FLinearColor::Yellow, 1.5f);
    }
}

void ACoreEngineVisualTestHUD::HandleHudMouseClicks(UEWorldSubsystemController& Ctrl) {
    bHudConsumedClickThisFrame = false;
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!PC->GetMousePosition(MouseX, MouseY)) {
        return;
    }

    const bool bLeftDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bRightDown = PC->IsInputKeyDown(EKeys::RightMouseButton);
    const bool bLeftJustPressed = bLeftDown && !bWasLeftMouseDown;
    const bool bRightJustPressed = bRightDown && !bWasRightMouseDown;
    bWasLeftMouseDown = bLeftDown;
    bWasRightMouseDown = bRightDown;

    // Check minimap bounds
    Vector2 ClickedUV;
    if (Ctrl.GetSandbox().GetMinimapSystem().ScreenPixelsToMinimapUV(Vector2(MouseX, MouseY), &ClickedUV)) {
        bHudConsumedClickThisFrame = true;
        if (bLeftJustPressed) {
            const Vector2 NewCamCore = Ctrl.GetSandbox().GetMinimapSystem().HandleLeftClickUV(
                ClickedUV, Ctrl.GetSandbox().GetGameState().GetMapSize());
            if (PC->GetPawn()) {
                const UEVec3 CamUE = Ctrl.GetSandbox().GetCoordinateBridge().CoreToUEPosition(NewCamCore, 4200.0f);
                PC->GetPawn()->SetActorLocation(FVector(CamUE.x - 1800.0f, CamUE.y, CamUE.z));
            }
        }
        if (bRightJustPressed) {
            Ctrl.GetSandbox().GetMinimapSystem().HandleRightClickUV(
                ClickedUV, Ctrl.GetSandbox().GetGameState());
        }
        return;
    }

    // Check GUI button rects
    for (const FClickableButtonRect& Btn : ButtonRectsThisFrame) {
        if (MouseX >= Btn.Min.X && MouseX <= Btn.Max.X && MouseY >= Btn.Min.Y && MouseY <= Btn.Max.Y) {
            bHudConsumedClickThisFrame = true;
            if (bLeftJustPressed) {
                Ctrl.GetGUI().ExecuteButtonAction(Btn.WidgetId);
            }
            return;
        }
    }
}

// ============================================================================
// ACoreEngineVisualTestGameMode
// ============================================================================

ACoreEngineVisualTestGameMode::ACoreEngineVisualTestGameMode() {
    HUDClass = ACoreEngineVisualTestHUD::StaticClass();
}

void ACoreEngineVisualTestGameMode::BeginPlay() {
    Super::BeginPlay();
    if (!GetWorld()) {
        return;
    }
    bool bFound = false;
    for (TActorIterator<ACoreEngineVisualTestActor> It(GetWorld()); It; ++It) {
        bFound = true;
        break;
    }
    if (!bFound) {
        GetWorld()->SpawnActor<ACoreEngineVisualTestActor>(
            ACoreEngineVisualTestActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    }
}
#endif
