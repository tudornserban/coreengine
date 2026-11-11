/**
 * CoreEngine C API — Flat C interface for foreign language interoperability.
 *
 * This is the ONLY public boundary between CoreEngine and other languages.
 * All types crossing this boundary are plain C types (no C++ classes).
 *
 * Compatible with: Python (ctypes/cffi), Rust (bindgen), Go (cgo),
 *                  Lua (luaL_cdecl), Unity C# (P/Invoke), any C FFI.
 *
 * Build as shared library:
 *   cmake -DBUILD_FFI=ON ..
 *   → libcoreengine_ffi.so / libcoreengine_ffi.dylib / coreengine_ffi.dll
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CE_FFI_VERSION_MAJOR 1
#define CE_FFI_VERSION_MINOR 1
#define CE_FFI_VERSION_PATCH 0
#define CE_FFI_VERSION       "1.1.0"

typedef enum CE_Result {
    CE_OK = 0,
    CE_ERR_NULL = 1,
    CE_ERR_NO_CHARGE = 2,
    CE_ERR_INVALID_TEAM = 3
} CE_Result;

/* ---- Opaque handle types ---- */
typedef void* CE_Engine;    /**< Opaque pointer to the engine instance */
typedef void* CE_Unit;      /**< Opaque pointer to a game unit */

/* ---- POD structs (safe to cross language boundaries) ---- */

typedef struct {
    float x;
    float y;
} CE_Vec2;

typedef struct {
    int   team;
    float x;
    float y;
    float vx;
    float vy;
    float hp;
    float maxHp;
    float radius;
} CE_UnitState;

typedef struct {
    float mapWidth;
    float mapHeight;
    int   unitCount;
    float matchTimeElapsed;
    int   matchOver;      /**< 0 = running, 1 = over */
    int   winner;         /**< 0 = none, 1 = team1, 2 = team2, 3 = draw */
} CE_GameSnapshot;

/* ---- Engine lifecycle ---- */

/** Create and initialize an engine instance.
 *  @param mapWidth   Arena width in world units
 *  @param mapHeight  Arena height in world units
 *  @param rngSeed    LCG seed for DeterministicRNG (0 is valid; replays match for the same seed)
 *  @return Opaque engine handle, or NULL on failure */
CE_Engine ce_engine_create(float mapWidth, float mapHeight, uint64_t rngSeed);

/** Destroy engine and free all resources. */
void ce_engine_destroy(CE_Engine engine);

/** Advance simulation by deltaTime seconds (fixed-timestep).
 *  @return 0 if engine stopped (match over or error), 1 if still running */
int ce_engine_update(CE_Engine engine, float deltaTime);

/** Reset engine to initial state (same map/seed). */
void ce_engine_reset(CE_Engine engine);

/* ---- Unit management ---- */

/** Spawn a unit at (x, y) for the given team (team ids **1** or **2** only).
 *  Requires a deployment charge unless you called `ce_timer_grant_charge`.
 *  @return 1 on success, 0 if no charge, invalid team, or null engine */
int ce_unit_spawn(CE_Engine engine, int teamId, float x, float y);

/** Get the total number of units currently in the simulation. */
int ce_unit_count(CE_Engine engine);

/** Read unit state at index into out.
 *  Units are ordered by insertion; dead units are removed.
 *  @return 1 on success, 0 if index out of range */
int ce_unit_get_state(CE_Engine engine, int index, CE_UnitState* out);

/** Set a unit's behavior by name (e.g. "Seek", "Flee", "Aggro").
 *  @return 1 on success, 0 if unit not found or behavior not registered */
int ce_unit_set_behavior(CE_Engine engine, int index, const char* behaviorName);

/* ---- Field queries ---- */

/** Query the influence field value at world position (x, y).
 *  Returns a signed value: positive = team1 dominates, negative = team2 dominates. */
float ce_field_query(CE_Engine engine, float x, float y);

/* ---- Game state snapshot ---- */

/** Fill a CE_GameSnapshot struct with current engine state.
 *  @return 1 on success, 0 on error */
int ce_snapshot_get(CE_Engine engine, CE_GameSnapshot* out);

/** Get a JSON string snapshot of the full game state.
 *  @param buffer     Output buffer to write into
 *  @param bufferSize Size of the output buffer
 *  @return number of bytes written (excluding null terminator), or -1 on error */
int ce_snapshot_get_json(CE_Engine engine, char* buffer, int bufferSize);

/* ---- Deployment timer ---- */

/** Query deployment charge availability.
 *  @return 1 if team has a charge ready, 0 otherwise */
int ce_timer_has_charge(CE_Engine engine, int teamId);

/** Get deployment charge progress in [0.0, 1.0] */
float ce_timer_get_progress(CE_Engine engine, int teamId);

/** Grant one deployment charge immediately (simulates 10 s of timer fill).
 *  @param teamId 1 or 2 only
 *  @return 1 on success, 0 on error */
int ce_timer_grant_charge(CE_Engine engine, int teamId);

/** Result code from the last spawn, timer, territory, or grant call.
 *  NULL handle -> CE_ERR_NULL */
int ce_last_result(CE_Engine engine);

/* ---- Territory ---- */

/** Get territory ownership fraction for a team in [0.0, 1.0] */
float ce_territory_get_pct(CE_Engine engine, int teamId);

/* ---- Behavior factory ---- */

/** Register a behavior name so it can be used with ce_unit_set_behavior().
 *  Game-specific behaviors must be registered before use.
 *  Built-in behaviors (Seek, Flee, Wander, Cohesion, Separation) are pre-registered. */
void ce_behavior_register(const char* name);

/** Get all registered behavior names as a comma-separated string.
 *  @param buffer     Output buffer
 *  @param bufferSize Buffer size
 *  @return bytes written, or -1 on error */
int ce_behavior_list(char* buffer, int bufferSize);

/* ---- Version ---- */

/** Returns the engine version string (e.g. "CoreEngine 1.0.0") */
const char* ce_version();

/** Returns the FFI package version string (CE_FFI_VERSION, e.g. "1.1.0") */
const char* ce_ffi_version(void);

#ifdef __cplusplus
}
#endif
