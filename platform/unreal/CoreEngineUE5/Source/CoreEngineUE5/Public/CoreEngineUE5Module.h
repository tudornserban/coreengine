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

#include "core/ai/BehaviorFactory.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma pop_macro("check")
#pragma pop_macro("TWO_PI")
#pragma pop_macro("HALF_PI")
#pragma pop_macro("PI")

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
