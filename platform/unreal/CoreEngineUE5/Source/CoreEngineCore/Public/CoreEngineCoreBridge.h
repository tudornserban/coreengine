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

#include <cstdint>
#include <memory>
#include <string>

#if defined(COREENGINE_WITH_UE5)
#include "CoreMinimal.h"
#else
#ifndef COREENGINECORE_API
#define COREENGINECORE_API
#endif
#endif

namespace CoreEngine {
namespace Unreal {

/// Exported ABI boundary between CoreEngineCore (RTTI-enabled pure C++17 simulation)
/// and CoreEngineUE5 (non-RTTI UCLASS Actors, HUD, and GameMode).
class COREENGINECORE_API FCoreEngineRuntimeBridge {
public:
    static void RegisterBuiltinBehaviors();

    FCoreEngineRuntimeBridge(float arenaWidth, float arenaHeight, uint64_t seed,
                             float worldScale, const UEVec3& originOffset);
    ~FCoreEngineRuntimeBridge();

    FCoreEngineRuntimeBridge(const FCoreEngineRuntimeBridge&) = delete;
    FCoreEngineRuntimeBridge& operator=(const FCoreEngineRuntimeBridge&) = delete;

    void Initialize(UESandboxPreset initialPreset, int unitsPerTeam);
    void Tick(float deltaSeconds);

    UEVec3 CoreToUEPosition(float coreX, float coreY, float heightOffsetUE) const;

    void InjectScreenMousePosition(float screenX, float screenY);
    void InjectWorldCameraRay(const UEVec3& rayOriginUE, const UEVec3& rayDirUE);
    void InjectMouseButtonState(int button, bool isDown);
    void InjectKeyState(int keyCode, bool isDown);

    bool ExecuteGuiButtonAction(const std::string& widgetId);

    const UE3DRenderFrame& GetLastRenderFrame() const;
    const UEGuiFrameModel& GetLastGuiFrameModel() const;

    void SetMinimapScreenRect(float originX, float originY, float widthPx, float heightPx);
    const UEMinimapFrame& GetLastMinimapFrame() const;
    bool ScreenPixelsToMinimapUV(float screenX, float screenY, float* outU, float* outV) const;
    UEVec3 HandleMinimapLeftClickUV(float u, float v, float cameraHeightUE);
    bool HandleMinimapRightClickUV(float u, float v);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Unreal
} // namespace CoreEngine
