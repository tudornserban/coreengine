#include "CoreEngineUE5Module.h"

#if defined(COREENGINE_WITH_UE5)
#include "CoreEngineCoreBridge.h"

void FCoreEngineUE5Module::StartupModule() {
    CoreEngine::Unreal::FCoreEngineRuntimeBridge::RegisterBuiltinBehaviors();
}

void FCoreEngineUE5Module::ShutdownModule() {
}

IMPLEMENT_MODULE(FCoreEngineUE5Module, CoreEngineUE5)
#endif
