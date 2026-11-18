# CoreEngine — Game Seeds Integration Notes

How each game project uses CoreEngine systems.

---

## VenusWars

VenusWars consumes CoreEngine as a pinned external C++ library. Do not symlink or copy `core/` into the game tree.

The engine supplies the fixed-step loop, playback, RNG, behavior factory, FSM, steering and influence, collision, map queries, commands, session lobby, projectiles, timed actions, placement footprints, resource claims, and a canonical snapshot. VenusWars keeps its own policy: deployment accounting, attribute curves, health-to-radius presentation, charge and Integrity costs, armor, spawn templates, objectives, rewards, progression, and UI.

Wire the game through `GameTitle::VenusWars` and `IDeterministicMatch`. Put scenario identity in `MatchSetup` and submit it before the lobby starts. `Configure` runs before the first tick. A full VenusWars encounter is validated in the VenusWars repository against a pinned CoreEngine revision, not in this repo.

Replay and rollback identity is the guarantee in [DETERMINISM.md](DETERMINISM.md): same binary, same platform, same compiler, and the same fixed timestep. Diagnostic JSON from `GameStateSerializer` is not that snapshot.

---

## DynamicGO

**Location:** `GameSeeds/DynamicGO/`

DynamicGO already has its own clean codebase. Integration path:

- `DeploymentTimer` and `ThinkTimer` are now in `core/gameloop/`
- `TerritoryCalculator` (2-team `Calculate` and N-team `UpdateGrid`), `PlacementValidator`, and `CombatManager` (influence attrition, direct attack, hybrid) live in CoreEngine
- `InfluenceMap` is replaced by `core/influence/TerritoryCalculator`
- `GeneticTrainer` / `BotPlayer` are in `core/ai_advanced/`
- Migrate `GameEngine.h` to use `RTSGameState` + `GameLogicEngine` from CoreEngine
- Keep DynamicGO-specific `AnchorUnit` as a game layer type (extends `RTSUnit`)
- Multiplayer goes through `core/session/`. Implement `IDeterministicMatch` for DynamicGo rules and pass an `IMatchFactory` to `SessionServer` (or `InProcessHost` for local play). `GameTitle::DynamicGo` is the queue key. Use `PlayMode::OfflineResolution` when the server should play the whole match from submitted plans, and `PlayMode::OnlineSync` for a live lockstep match. AscendantExpanse (`GameTitle::AscendantExpanse`) and ZodiacWars (`GameTitle::ZodiacWars`) use the same session API when those games take it up; this repo does not ship their rules.

---

## AscendantExpanse

**Location:** `GameSeeds/AscendantExpanse/`

Currently design-only. Implementation path when ready:

- Uses **all** CoreEngine systems
- `Pilot.SunSign` and `Pilot.AscendantSign` → registered as custom behaviors in `BehaviorFactory`
- `ActivityZone` (influence zone) → `InfluenceFieldItem` attractors/repulsors
- Faction-specific behavior tuning → `IBehavior::GetParameters()` map
- 12 factions → 12 behavior presets registered at startup
- `VehicleCraftingSystem` → game layer (not in CoreEngine)
- Map: 3D topographical maps → add `Vector3` support in `core/math/` when needed

---

## RationalInsanity

**Location:** `GameSeeds/RationalInsanity/`

Currently design-only. The key system is the **magic dust swap mechanic**:

```
Fairy flies to Construct → player selects new Dust cartridge →
  BehaviorFactory::Create("Aggro-Dust") → StateMachine::SetState("Aggro-Dust")
  (game-registered name — **not** a CoreEngine built-in)
```

CoreEngine systems used:
- `BehaviorFactory` — runtime behavior creation by name (the core of the mechanic)
- `StateMachine::SetState()` — instant behavior swap without destroying the state machine
- `IBehavior::OnEnter/OnExit` — called automatically on swap
- Dust cartridge = a registered behavior name string
- Pre-game brewing = composing behavior parameter maps before battle

The game layer registers all Dust behaviors at startup:
```cpp
BehaviorFactory::Register("Aggro-Dust",   []{ return std::make_unique<AggroDustBehavior>(); });
BehaviorFactory::Register("Guard-Dust",   []{ return std::make_unique<GuardDustBehavior>(); });
BehaviorFactory::Register("March-Dust",   []{ return std::make_unique<MarchDustBehavior>(); });
BehaviorFactory::Register("Scavenge-Dust",[]{ return std::make_unique<ScavengeDustBehavior>(); });
```

---

## SwordDistrict (future)

**Location:** `GameSeeds/SwordDistrict/`

Not yet assessed. When ready, evaluate which CoreEngine systems apply.
