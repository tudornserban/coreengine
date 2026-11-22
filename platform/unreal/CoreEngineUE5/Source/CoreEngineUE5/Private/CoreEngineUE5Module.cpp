#include "CoreEngineUE5Module.h"

#if defined(COREENGINE_WITH_UE5)
void FCoreEngineUE5Module::StartupModule() {
    BehaviorFactory::RegisterBuiltins();
}

void FCoreEngineUE5Module::ShutdownModule() {
}

IMPLEMENT_MODULE(FCoreEngineUE5Module, CoreEngineUE5)
#endif
