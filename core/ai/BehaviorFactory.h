#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>
#include "../interfaces/IBehavior.h"

/// Factory for creating behaviors by name at runtime.
///
/// Enables the "magic dust swap" mechanic from RationalInsanity:
/// the Fairy swaps a Construct's behavior by calling SetState() on its StateMachine
/// with a behavior key. Game-specific behaviors are registered at startup.
///
/// Usage:
///   BehaviorFactory::Register("Aggro",  []{ return std::make_unique<AggroBehavior>(); });
///   BehaviorFactory::Register("Guard",  []{ return std::make_unique<GuardBehavior>(); });
///
///   auto b = BehaviorFactory::Create("Aggro");
///   stateMachine.AddState("Aggro", std::move(b));
///   stateMachine.SetState("Aggro");
///
/// All core behaviors are pre-registered; game-specific behaviors are registered
/// by each game project's initialization code.
class BehaviorFactory {
public:
    using Creator = std::function<std::unique_ptr<IBehavior>()>;

    /// Register a behavior type with a string key.
    /// Re-registration overwrites the previous creator (last write wins).
    static void Register(const std::string& key, Creator creator);

    /// Create a new instance of the behavior registered under key.
    /// Returns nullptr if the key has not been registered.
    static std::unique_ptr<IBehavior> Create(const std::string& key);

    /// Returns true if key is registered.
    static bool IsRegistered(const std::string& key);

    /// Returns all registered behavior keys (for editors and debug UIs).
    static std::vector<std::string> GetRegisteredKeys();

    /// Remove a registered behavior (e.g. for test teardown).
    static void Unregister(const std::string& key);

    /// Register all built-in core behaviors.
    /// Call once during engine initialization.
    static void RegisterBuiltins();

private:
    static std::map<std::string, Creator>& Registry();
};
