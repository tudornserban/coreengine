# Module: `core.session`

Manifest: [docs/agentic/modules/core.session.module.json](../../docs/agentic/modules/core.session.module.json)

## Purpose

Multiplayer lobby for games that sit on CoreEngine. The module owns the client-server session, not the game rules and not sockets.

`core/` stays free of platform headers, so there is no TCP or UDP here. `ISessionTransport` is the byte pipe. `InProcessTransport` / `InProcessHost` are the reference client and server in one process: every message is encoded before it is delivered. A later platform adapter implements the same interface with real sockets and calls `SessionServer` on the server clock.

Two play modes, selected per lobby:

| Mode | What the server does |
|------|----------------------|
| `OfflineResolution` | Matchmaking (or a lobby) collects one `DecisionPlan` per player. `ResolveOffline` runs `IDeterministicMatch` to the end. Every client receives the same `MatchOutcome`. |
| `OnlineSync` | Server-authoritative lockstep at `LatencyProfile::tickHz` (default 30, the engine logic rate). Clients should lead by `InputDelayTicksForRtt` so a command is waiting when its tick is simulated. A late command inside `rollbackWindowTicks` restores a snapshot and resimulates. Older commands are dropped. Presentation of remote players waits `InterpolationDelayMs` (half the worst RTT in the match). |

DynamicGo is the first game meant to supply `IDeterministicMatch`. AscendantExpanse and ZodiacWars use the same `GameTitle` values when they adopt it. The session module does not include those games.

## Public surface (`exports`)

```cpp
#include "core/session/InProcessHost.h"
#include "core/session/IDeterministicMatch.h"
#include "core/session/OfflineResolver.h"
#include "core/session/Latency.h"
```

| Symbol | Header |
|--------|--------|
| `GameTitle`, `PlayMode`, `LobbyView`, `DecisionPlan`, `PlayerCommand`, `MatchOutcome` | `SessionTypes.h` |
| `IDeterministicMatch`, `IMatchFactory` | `IDeterministicMatch.h` |
| `ISessionTransport`, `InProcessTransport` | `ISessionTransport.h` |
| `SessionServer`, `SessionClient`, `InProcessHost` | `SessionServer.h`, `SessionClient.h`, `InProcessHost.h` |
| `OnlineMatch` | `OnlineMatch.h` |
| `ResolveOffline` | `OfflineResolver.h` |
| `LatencyProfile`, `InputDelayTicksForRtt`, `InterpolationDelayMs` | `Latency.h` |

`SessionServer::Receive` trusts the player id argument, which is the authenticated connection. Plan and command player ids inside the payload are rewritten to that connection so a client cannot submit another player's move.

`AdvanceOnlineMatches` is server-driven. Clients cannot advance the clock.

Input delay for a match is computed from the **worst** reported RTT so the slower peer's commands have time to arrive:

`ceil((rttMs / 2) / tickMs) + jitterMarginTicks`, clamped to `[minInputDelayTicks, maxInputDelayTicks]`.

At 30 Hz a 100 ms RTT is 3 ticks (33 ms ticks, 50 ms one way, two ticks, plus one tick of jitter margin).

## Dependencies (`requires`)

None. Game rules are injected through `IMatchFactory`.

## Invariants

- Same seed, same player ids, and the same commands produce the same `MatchOutcome::stateHash` from `ResolveOffline` and from an online match that actually applied those commands.
- Offline resolution applies at most one command per player per tick (last write wins), in ascending player id order, then `Step`.
- Online rollback closes when the match finishes or the command is older than `rollbackWindowTicks`.
- `IDeterministicMatch::Restore` must leave the sim unchanged when it returns false.
- Not thread-safe. Pump one host on one thread.
- No platform or FFI includes.

## Tests

```bash
./scripts/run-session-module-tests.sh
```

| File | Role |
|------|------|
| `tests/UnitTests/Session/SessionTest.cpp` | Lobby, matchmaking, offline replay, lockstep, rollback, RTT, codec |
| `tests/UnitTests/Session/SessionHeaderCompileTest.cpp` | Public headers compile together |

## Change checklist

- [ ] Bump manifest patch/minor/major per [AGENTS.md](../../AGENTS.md)
- [ ] `./scripts/check-module-deps.sh` and `python3 scripts/test_check_module_deps.py`
- [ ] `./scripts/run-session-module-tests.sh`
