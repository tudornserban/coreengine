# Determinism

CoreEngine matches a replay when every participant runs the **same binary**, built by the **same compiler**, on the **same platform**, and advances the simulation with the **same fixed timestep**.

That match is exact for the canonical state hash. It is not a promise across operating systems, compilers, or compiler versions. The engine does not use fixed-point arithmetic.

## What is compared

`RTSGameState::CaptureCanonicalState` writes schema 1 text (`CESTATE 1`). Floats are hex so the same platform can round-trip them. `CanonicalStateHash` is FNV-1a 64 over that text. Records are ordered by entity id. Addresses and unordered-map iteration are not part of the text.

The text includes the tick, RNG seed and state, the next ids, the lifecycle log, unit kinematics and participation, behavior parameters, transition timers, projectiles, timed actions, id-keyed combat cooldowns, map resource amounts, collision immunity by entity id, and optional game-extension bytes.

`GameStateSerializer` JSON is a diagnostic view. `Deserialize` does not restore a match. Paused `Update` does not advance the tick, the RNG, or unit motion, and it leaves the last diagnostics in place. Turning diagnostics on does not change the canonical hash.

## What a game still owns

Rules identity is a single token stored with the snapshot (the default is `unspecified`). Games supply firing policy, armor, costs, health-to-radius presentation, and the extension bytes. An extension is accepted only after its validate callback returns true without mutating the world. Restore requires the same unit topology the game already constructed. It does not respawn units.

## Session

`IDeterministicMatch::Step` is one simulation tick. `ResolveOffline` applies commands for a tick, then steps once. Online rollback depends on this canonical snapshot. The session module does not claim cross-platform checksums.
