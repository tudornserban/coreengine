# CoreEngine Changelog

This document tracks changes made to CoreEngine, including those driven by game-seed integration feedback.  
Each entry links back to the originating proposal document where applicable.

---

## 2026-03-25 — FFI 1.3.0 / core.gamestate 1.3.0: behavior acceleration registration

### F1.2 — `ce_behavior_register_accel` and hub acceleration path

- `ce_behavior_register_accel` registers a C callback as a direct-acceleration behavior (`CE_AccelFn` + optional `userdata`).
- `ce_behavior_register(const char*)` documented as deprecated no-op (symbol retained for ABI).
- `RTSGameState::Update` applies `CalculateAcceleration` when `GenerateFieldItems` is empty, then adds static field forces (map edges).
- `core.gamestate` manifest **1.3.0**; `CE_FFI_VERSION` **1.3.0**.

---

## 2026-03-25 — FFI 1.2.0: reset keeps RNG seed

### F1.3 — `ce_engine_reset` preserves create seed

- `EngineContext` stores the `rngSeed` from `ce_engine_create`; reset rebuilds `RTSGameState` with that seed instead of defaulting to 0.
- New `ce_engine_get_seed` reads the live `DeterministicRNG` seed (null engine → `0`).
- `CE_FFI_VERSION` bumped to **1.2.0**; bindings and docs updated.

---

## 2026-03-14 — RationalInsanity Bootstrap Sprint

Source: `GameSeeds/RationalInsanity/docs/strategic/engine-proposals.md`

### EP-001: VenusWars → CoreEngine Migration Cleanup ✅ (bootstrap)

**Files changed:** `RTSGameState.h/.cpp`, `RTSUnit.h/.cpp`, `CombatManager.h/.cpp`, `PlacementValidator.h/.cpp`, `MapObstacleComponent.h`, `DeploymentTimer.cpp`, `ThinkTimer.cpp`

Fixes applied during the RationalInsanity bootstrap to make CoreEngine compile cleanly:

- `RTSGameState` fully migrated from `PotentialFieldItem` to `InfluenceFieldItem`
- `RTSUnit` rewritten: removed `PotentialFieldController` / `PFCParameters` dependency (superseded by `InfluenceField::CalculateTotalForce()` in `RTSGameState::Update()`)
- `CombatManager` and `PlacementValidator` stubbed — game-specific logic belongs in the game layer
- `DeploymentTimer.cpp` and `ThinkTimer.cpp` syntax errors (missing closing braces) corrected
- `MapObstacleComponent` updated from `PotentialFieldItem` to `InfluenceFieldItem`

---

### EP-003: `RTSGameState` Perception Radius Setter/Getter ✅

**File changed:** `core/gamestate/RTSGameState.h`

`perceptionRadius_` was hardcoded at 300.0f in the constructor with no public accessor. Added:

```cpp
void SetPerceptionRadius(float radius) { perceptionRadius_ = radius; }
float GetPerceptionRadius() const { return perceptionRadius_; }
```

This is a safe, non-breaking additive change. Required before Epic 008 (per-construct perception tuning).

---

### EP-004: `StateMachine::AddState()` Duplicate Guard ✅

**File changed:** `GameSeeds/RationalInsanity/src/ConstructUnit.cpp`

`StateMachine::AddState()` already silently overwrites duplicate keys via `states_[name] = std::move(behavior)`. The game-layer guard in `ConstructUnit::SwapDust()` that manually checked `GetStates().find(key)` before calling `AddState()` was a leaky abstraction compensating for a non-existent problem.

Removed the redundant guard. `SwapDust()` now unconditionally calls `AddState()` then `SetState()`.

---

## Resolved Without Changes

| Proposal | Resolution |
|----------|-----------|
| EP-002: `BehaviorFactory::IsRegistered()` | Already present in `BehaviorFactory.h` — no action needed |
| EP-005: `RTSUnit::GetStateMachine()` const correctness | Already correctly exposed both const and non-const overloads — no action needed |
