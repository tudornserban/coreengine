--[[
CoreEngine Lua bindings.

Uses LuaJIT FFI (or luaffi for standard Lua).

Deployment timers, spawn, and territory queries accept team ids **1** and **2** only (see ce_last_result).

Usage:
    local ffi  = require "ffi"
    local ce   = require "engine"

    local eng  = ce.create(1600, 900, 0)
    while not ce.has_charge(eng, 1) do
        ce.update(eng, 1/30)
    end
    ce.spawn(eng, 1, 400, 450)
    print(ce.snapshot_json(eng))
    ce.destroy(eng)
--]]

local ffi = require("ffi")

ffi.cdef[[
    void* ce_engine_create(float mapWidth, float mapHeight, uint64_t rngSeed);
    void  ce_engine_destroy(void* engine);
    int   ce_engine_update(void* engine, float deltaTime);
    void  ce_engine_reset(void* engine);
    int   ce_unit_spawn(void* engine, int teamId, float x, float y);
    int   ce_unit_count(void* engine);
    int   ce_unit_set_behavior(void* engine, int index, const char* name);
    float ce_field_query(void* engine, float x, float y);
    int   ce_snapshot_get_json(void* engine, char* buf, int size);
    int   ce_timer_has_charge(void* engine, int teamId);
    float ce_timer_get_progress(void* engine, int teamId);
    int   ce_timer_grant_charge(void* engine, int teamId);
    int   ce_last_result(void* engine);
    float ce_territory_get_pct(void* engine, int teamId);
    const char* ce_version(void);
    const char* ce_ffi_version(void);
]]

local lib = ffi.load("coreengine_ffi")

local M = {}

function M.create(w, h, seed)   return lib.ce_engine_create(w, h, seed or 0) end
function M.destroy(e)            lib.ce_engine_destroy(e) end
function M.update(e, dt)         return lib.ce_engine_update(e, dt) end
function M.reset(e)              lib.ce_engine_reset(e) end
function M.spawn(e, team, x, y)  return lib.ce_unit_spawn(e, team, x, y) end
function M.has_charge(e, team)   return lib.ce_timer_has_charge(e, team) ~= 0 end
function M.grant_charge(e, team) return lib.ce_timer_grant_charge(e, team) ~= 0 end
function M.last_result(e)        return lib.ce_last_result(e) end
function M.unit_count(e)         return lib.ce_unit_count(e) end
function M.set_behavior(e, i, n) return lib.ce_unit_set_behavior(e, i, n) end
function M.field_query(e, x, y)  return lib.ce_field_query(e, x, y) end
function M.territory(e, team)    return lib.ce_territory_get_pct(e, team) end
function M.version()             return ffi.string(lib.ce_version()) end
function M.ffi_version()         return ffi.string(lib.ce_ffi_version()) end

function M.snapshot_json(e)
    local buf = ffi.new("char[65536]")
    local n = lib.ce_snapshot_get_json(e, buf, 65536)
    if n < 0 then return nil end
    return ffi.string(buf, n)
end

return M
