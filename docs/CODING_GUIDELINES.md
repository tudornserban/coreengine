# CoreEngine — Coding Guidelines

Adapted from VenusWars CODING_GUIDELINES.md. Follow these rules in all `core/` files.

---

## Non-negotiable rules

1. **No platform headers in `core/`** — Zero `<SDL.h>`, `<raylib.h>`, UE5 macros, etc.
2. **C++17 STL only** — `<vector>`, `<memory>`, `<string>`, `<functional>`, `<map>`, `<cmath>`, `<algorithm>`
3. **`ARCHITECTURE.md` must stay current** — Update it with every subsystem change.
4. **Include paths** — Always relative; no absolute paths.

---

## Style

- **Indentation:** 4 spaces, no tabs
- **Braces:** Allman style (opening brace on its own line)
- **Naming:**
  - Files and classes: `PascalCase`
  - Member variables: `camelCase_` (trailing underscore)
  - Local variables and parameters: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
- **Smart pointers:** `unique_ptr` preferred; `shared_ptr` only when ownership is genuinely shared
- **No raw `new`/`delete`** — use `std::make_unique`, `std::make_shared`
- **`const` correctness** — mark all read-only methods `const`

---

## Memory

```cpp
// Correct
auto unit = std::make_unique<RTSUnit>(pos, team);
gameState.AddUnit(std::move(unit));

// Wrong
RTSUnit* unit = new RTSUnit(pos, team);  // NEVER
```

---

## Interfaces

All public API types live in `core/interfaces/`. Concrete types depend on interfaces, not vice versa.

```cpp
// Correct — depends on interface
void DoSomething(IGameUnit* unit);

// Wrong — depends on concrete type
void DoSomething(RTSUnit* unit);  // avoid outside collision/gamestate layer
```

---

## Behaviors

Every behavior must:
- Implement `GetName()` returning a unique string literal
- Implement `GetParameters()` returning a `std::map<std::string, float>`
- Be registered in `BehaviorFactory` with the same name string
- Not store mutable state that depends on a specific unit instance (behaviors are shared)

---

## Comments

Only explain **why**, not **what**. No narration comments.

```cpp
// Correct — explains non-obvious constraint
// DynamicThreatAssessment requires velocity data that isn't always available.

// Wrong — narrates the code
// Set the velocity      ← NEVER
velocity_ = newVel;
```

---

## FFI boundary

The C API in `ffi/c_api.h` and `ffi/c_api.cpp` must:
- Use only plain C types (no `std::`, no C++ classes in function signatures)
- Validate all input pointers before dereferencing
- Catch all exceptions before returning to C callers
- Never allocate memory that the caller is responsible for freeing (use engine-managed buffers)

---

## Modification zones

| Zone | Files | Rule |
|------|-------|------|
| Ultra-high | `InfluenceFieldCalculator.*`, `InfluenceFieldItem.*` | Always ask human |
| High | `StateMachine.*`, `CollisionSystem.*`, `RTSGameState.*`, `RTSUnit.*` | Always ask human |
| Medium | Behaviors, Conditions, Map, Combat | Ask if non-trivial |
| Low | New behaviors, platform/ adapters, ffi/ stubs | Proceed autonomously |
