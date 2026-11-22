#pragma once

#pragma push_macro("PI")
#undef PI
#pragma push_macro("HALF_PI")
#undef HALF_PI
#pragma push_macro("TWO_PI")
#undef TWO_PI
#pragma push_macro("check")
#undef check
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wundef"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4458)
#endif

#include "platform/unreal/UECoordinateBridge.h"
#include "platform/unreal/UEDefaultGUI.h"
#include "platform/unreal/UEGenericCharacter.h"
#include "platform/unreal/UEGenericInventory.h"
#include "platform/unreal/UEInputReader.h"
#include "platform/unreal/UEMinimapSystem.h"
#include "platform/unreal/UEPlaceholder3DSandbox.h"
#include "platform/unreal/UERenderer.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma pop_macro("check")
#pragma pop_macro("TWO_PI")
#pragma pop_macro("HALF_PI")
#pragma pop_macro("PI")

#include <memory>
#include <string>
#include <vector>

#if defined(COREENGINE_WITH_UE5)
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#endif

namespace CoreEngine {
namespace Unreal {

/// High-level UE5 World Subsystem controller owning the CoreEngine 3D Sandbox and Default GUI.
/// In UE5, UCoreEngineWorldSubsystem embeds this controller so any Actor, Character, Component,
/// or Blueprint can access the simulation with zero boilerplate.
class UEWorldSubsystemController {
private:
    UEPlaceholder3DSandbox sandbox_;
    UEDefaultGUI gui_;
    bool autoTickEnabled_ = true;

public:
    explicit UEWorldSubsystemController(Vector2 arenaSize = Vector2(800.0f, 600.0f),
                                        uint64_t seed = 42,
                                        UECoordinateConfig coordConfig = UECoordinateConfig{})
        : sandbox_(arenaSize, seed, std::move(coordConfig))
        , gui_(&sandbox_) {}

    void Initialize(UESandboxPreset initialPreset = UESandboxPreset::SingleUnitBehaviorLab,
                    int unitsPerTeam = 4) {
        sandbox_.LoadPreset(initialPreset, unitsPerTeam);
        gui_.BuildFrameModel();
    }

    void Tick(float deltaSeconds) {
        if (!autoTickEnabled_) {
            return;
        }
        gui_.ProcessKeyboardShortcuts(sandbox_.GetInputReader());
        sandbox_.TickFrame(deltaSeconds);
        gui_.BuildFrameModel();
    }

    void SetAutoTickEnabled(bool enabled) { autoTickEnabled_ = enabled; }
    bool IsAutoTickEnabled() const { return autoTickEnabled_; }

    UEPlaceholder3DSandbox& GetSandbox() { return sandbox_; }
    const UEPlaceholder3DSandbox& GetSandbox() const { return sandbox_; }

    UEDefaultGUI& GetGUI() { return gui_; }
    const UEDefaultGUI& GetGUI() const { return gui_; }
};

#if defined(COREENGINE_WITH_UE5)
/// Helper to draw all 3D placeholder primitives from UERenderer using UE5's built-in 3D debug renderer.
/// Works immediately in any UE5 level without requiring custom static mesh .uasset files.
inline void DrawUE5Placeholder3DFrame(UWorld* World, const UE3DRenderFrame& Frame, float LifeTime = -1.0f) {
    if (!World) {
        return;
    }
    for (const UE3DPrimitive& Prim : Frame.primitives) {
        const FColor Col(Prim.color.r, Prim.color.g, Prim.color.b, Prim.color.a);
        const FVector Loc(Prim.transform.location.x, Prim.transform.location.y, Prim.transform.location.z);
        const FQuat Rot = FRotator(Prim.transform.rotation.pitch,
                                   Prim.transform.rotation.yaw,
                                   Prim.transform.rotation.roll).Quaternion();

        switch (Prim.kind) {
            case UE3DPrimitiveKind::Capsule:
                DrawDebugCapsule(World, Loc, Prim.halfHeight, Prim.radius, Rot, Col, false, LifeTime, 0, Prim.thickness);
                break;
            case UE3DPrimitiveKind::Cylinder: {
                const FVector Base = Loc - FVector(0.0f, 0.0f, Prim.halfHeight);
                const FVector Top  = Loc + FVector(0.0f, 0.0f, Prim.halfHeight);
                DrawDebugCylinder(World, Base, Top, Prim.radius, 16, Col, false, LifeTime, 0, Prim.thickness);
                break;
            }
            case UE3DPrimitiveKind::Cone: {
                const FVector Dir = Rot.GetForwardVector();
                DrawDebugCone(World, Loc, -Dir, Prim.halfHeight * 2.0f, 0.45f, 0.45f, 12, Col, false, LifeTime, 0, Prim.thickness);
                break;
            }
            case UE3DPrimitiveKind::Sphere:
            case UE3DPrimitiveKind::WireSphere:
                DrawDebugSphere(World, Loc, Prim.radius, 16, Col, false, LifeTime, 0, Prim.thickness);
                break;
            case UE3DPrimitiveKind::Box: {
                const FVector Extent(Prim.endPoint.x, Prim.endPoint.y, Prim.endPoint.z);
                DrawDebugBox(World, Loc, Extent, Col, false, LifeTime, 0, Prim.thickness);
                break;
            }
            case UE3DPrimitiveKind::Arrow3D: {
                const FVector End(Prim.endPoint.x, Prim.endPoint.y, Prim.endPoint.z);
                DrawDebugDirectionalArrow(World, Loc, End, 45.0f, Col, false, LifeTime, 0, Prim.thickness + 1.0f);
                break;
            }
            case UE3DPrimitiveKind::Ring3D:
                DrawDebugCircle(World, Loc, Prim.radius, 32, Col, false, LifeTime, 0, Prim.thickness,
                                FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
                break;
            case UE3DPrimitiveKind::Quad3D: {
                const FVector Extent(Prim.radius, Prim.halfHeight, 2.0f);
                DrawDebugSolidBox(World, Loc, Extent, Col, false, LifeTime, 0);
                break;
            }
            case UE3DPrimitiveKind::Line3D: {
                const FVector End(Prim.endPoint.x, Prim.endPoint.y, Prim.endPoint.z);
                DrawDebugLine(World, Loc, End, Col, false, LifeTime, 0, Prim.thickness);
                break;
            }
            case UE3DPrimitiveKind::TextBadge3D:
                DrawDebugString(World, Loc, UTF8_TO_TCHAR(Prim.text.c_str()), nullptr, Col, 0.0f, true);
                break;
        }
    }
}
#endif

} // namespace Unreal
} // namespace CoreEngine
