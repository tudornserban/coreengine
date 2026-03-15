#include "BehaviorFactory.h"
#include "Behaviors/SeekBehavior.h"
#include "Behaviors/FleeBehavior.h"
#include "Behaviors/WanderBehavior.h"
#include "Behaviors/CohesionBehavior.h"
#include "Behaviors/SeparationBehavior.h"
#include <algorithm>

std::map<std::string, BehaviorFactory::Creator>& BehaviorFactory::Registry()
{
    static std::map<std::string, Creator> registry;
    return registry;
}

void BehaviorFactory::Register(const std::string& key, Creator creator)
{
    Registry()[key] = std::move(creator);
}

std::unique_ptr<IBehavior> BehaviorFactory::Create(const std::string& key)
{
    auto& reg = Registry();
    auto it = reg.find(key);
    if (it == reg.end()) return nullptr;
    return it->second();
}

bool BehaviorFactory::IsRegistered(const std::string& key)
{
    return Registry().count(key) > 0;
}

std::vector<std::string> BehaviorFactory::GetRegisteredKeys()
{
    std::vector<std::string> keys;
    for (const auto& kv : Registry()) {
        keys.push_back(kv.first);
    }
    return keys;
}

void BehaviorFactory::Unregister(const std::string& key)
{
    Registry().erase(key);
}

void BehaviorFactory::RegisterBuiltins()
{
    Register("Seek",       []{ return std::make_unique<SeekBehavior>(); });
    Register("Flee",       []{ return std::make_unique<FleeBehavior>(); });
    Register("Wander",     []{ return std::make_unique<WanderBehavior>(); });
    Register("Cohesion",   []{ return std::make_unique<CohesionBehavior>(); });
    Register("Separation", []{ return std::make_unique<SeparationBehavior>(); });
}
