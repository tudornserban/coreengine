#pragma once

#include "core/ai/BehaviorFactory.h"

#if defined(COREENGINE_WITH_UE5)
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class COREENGINEUE5_API FCoreEngineUE5Module : public IModuleInterface {
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
#else
/// Standalone / headless stub when compiling outside UnrealBuildTool.
class FCoreEngineUE5Module {
public:
    void StartupModule() { BehaviorFactory::RegisterBuiltins(); }
    void ShutdownModule() {}
};
#endif
