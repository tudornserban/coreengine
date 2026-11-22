#include "CoreEngineCoreBridge.h"

#if defined(COREENGINE_WITH_UE5)
#include "Modules/ModuleManager.h"
IMPLEMENT_MODULE(FDefaultModuleImpl, CoreEngineCore)
#endif

namespace CoreEngine {
namespace Unreal {

struct FCoreEngineRuntimeBridge::Impl {
    UEPlaceholder3DSandbox sandbox;
    UEDefaultGUI gui;

    Impl(Vector2 arenaSize, uint64_t seed, UECoordinateConfig coordConfig)
        : sandbox(arenaSize, seed, std::move(coordConfig))
        , gui(&sandbox) {}
};

void FCoreEngineRuntimeBridge::RegisterBuiltinBehaviors() {
    BehaviorFactory::RegisterBuiltins();
}

FCoreEngineRuntimeBridge::FCoreEngineRuntimeBridge(float arenaWidth, float arenaHeight, uint64_t seed,
                                                   float worldScale, const UEVec3& originOffset) {
    UECoordinateConfig cfg;
    cfg.worldScale = (worldScale > 0.0f) ? worldScale : 10.0f;
    cfg.originOffset = originOffset;
    impl_ = std::make_unique<Impl>(Vector2(arenaWidth, arenaHeight), seed, std::move(cfg));
}

FCoreEngineRuntimeBridge::~FCoreEngineRuntimeBridge() = default;

void FCoreEngineRuntimeBridge::Initialize(UESandboxPreset initialPreset, int unitsPerTeam) {
    impl_->sandbox.LoadPreset(initialPreset, unitsPerTeam);
    impl_->gui.BuildFrameModel();
}

void FCoreEngineRuntimeBridge::Tick(float deltaSeconds) {
    impl_->gui.ProcessKeyboardShortcuts(impl_->sandbox.GetInputReader());
    impl_->sandbox.TickFrame(deltaSeconds);
    impl_->gui.BuildFrameModel();
}

UEVec3 FCoreEngineRuntimeBridge::CoreToUEPosition(float coreX, float coreY, float heightOffsetUE) const {
    return impl_->sandbox.GetCoordinateBridge().CoreToUEPosition(Vector2(coreX, coreY), heightOffsetUE);
}

void FCoreEngineRuntimeBridge::InjectScreenMousePosition(float screenX, float screenY) {
    impl_->sandbox.GetInputReader().InjectScreenMousePosition(Vector2(screenX, screenY));
}

void FCoreEngineRuntimeBridge::InjectWorldCameraRay(const UEVec3& rayOriginUE, const UEVec3& rayDirUE) {
    impl_->sandbox.GetInputReader().InjectWorldCameraRay(
        rayOriginUE, rayDirUE, impl_->sandbox.GetCoordinateBridge());
}

void FCoreEngineRuntimeBridge::InjectMouseButtonState(int button, bool isDown) {
    impl_->sandbox.GetInputReader().InjectMouseButtonState(button, isDown);
}

void FCoreEngineRuntimeBridge::InjectKeyState(int keyCode, bool isDown) {
    impl_->sandbox.GetInputReader().InjectKeyState(keyCode, isDown);
}

bool FCoreEngineRuntimeBridge::ExecuteGuiButtonAction(const std::string& widgetId) {
    return impl_->gui.ExecuteButtonAction(widgetId);
}

const UE3DRenderFrame& FCoreEngineRuntimeBridge::GetLastRenderFrame() const {
    return impl_->sandbox.GetRenderer().GetLastRenderFrame();
}

const UEGuiFrameModel& FCoreEngineRuntimeBridge::GetLastGuiFrameModel() const {
    return impl_->gui.GetLastFrameModel();
}

void FCoreEngineRuntimeBridge::SetMinimapScreenRect(float originX, float originY,
                                                    float widthPx, float heightPx) {
    auto& cfg = impl_->sandbox.GetMinimapSystem().GetConfig();
    cfg.screenOriginX = originX;
    cfg.screenOriginY = originY;
    cfg.screenWidthPx = widthPx;
    cfg.screenHeightPx = heightPx;
}

const UEMinimapFrame& FCoreEngineRuntimeBridge::GetLastMinimapFrame() const {
    return impl_->sandbox.GetMinimapSystem().GetLastFrame();
}

bool FCoreEngineRuntimeBridge::ScreenPixelsToMinimapUV(float screenX, float screenY,
                                                       float* outU, float* outV) const {
    Vector2 uv;
    if (!impl_->sandbox.GetMinimapSystem().ScreenPixelsToMinimapUV(Vector2(screenX, screenY), &uv)) {
        return false;
    }
    if (outU) *outU = uv.x;
    if (outV) *outV = uv.y;
    return true;
}

UEVec3 FCoreEngineRuntimeBridge::HandleMinimapLeftClickUV(float u, float v, float cameraHeightUE) {
    const Vector2 newCamCore = impl_->sandbox.GetMinimapSystem().HandleLeftClickUV(
        Vector2(u, v), impl_->sandbox.GetGameState().GetMapSize());
    return impl_->sandbox.GetCoordinateBridge().CoreToUEPosition(newCamCore, cameraHeightUE);
}

bool FCoreEngineRuntimeBridge::HandleMinimapRightClickUV(float u, float v) {
    return impl_->sandbox.GetMinimapSystem().HandleRightClickUV(
        Vector2(u, v), impl_->sandbox.GetGameState());
}

} // namespace Unreal
} // namespace CoreEngine
