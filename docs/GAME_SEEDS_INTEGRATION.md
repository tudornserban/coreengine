# CoreEngine — Game Seeds Integration Notes

How each game project uses CoreEngine systems.

---

## VenusWars

**Location:** `VenusWars/` (CMake project and sources at repository root)

VenusWars is the original source of most `core/` systems. After extraction:

- **Replace** `src/Core/` with a symlink or copy of `CoreEngine/core/`
- **Keep** `src/VenusWars/` — game-specific behaviors (`ExplorationBehavior`, `CombatBehavior`, `DefensiveBehavior`), unit types (`VenusWarsUnit`), and game states
- **Keep** `src/Platform/` — SDL2 renderer and input (now equivalent to `CoreEngine/platform/sdl2/`)
- **Replace** `PotentialFieldItem` references with `InfluenceFieldItem` (same API, renamed)
- Register VenusWars-specific behaviors via `BehaviorFactory::Register("Combat", ...)`

---

## DynamicGO

**Location:** `GameSeeds/DynamicGO/`

DynamicGO already has its own clean codebase. Integration path:

- `DeploymentTimer` and `ThinkTimer` are now in `CoreEngine/core/gameloop/`
- `TerritoryCalculator`, `PlacementValidator`, `CombatManager` are in CoreEngine
- `InfluenceMap` is replaced by `CoreEngine/core/influence/TerritoryCalculator`
- `GeneticTrainer` / `BotPlayer` are in `CoreEngine/core/ai_advanced/`
- Migrate `GameEngine.h` to use `RTSGameState` + `GameLogicEngine` from CoreEngine
- Keep DynamicGO-specific `AnchorUnit` as a game layer type (extends `RTSUnit`)

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
  BehaviorFactory::Create("Aggro") → StateMachine::SetState("Aggro")
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
