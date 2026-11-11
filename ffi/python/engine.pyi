"""
CoreEngine Python bindings stub.

Usage with ctypes:

    from ctypes import CDLL, c_float, c_int, c_uint64, c_char_p, POINTER, Structure
    import ctypes

    lib = CDLL("libcoreengine_ffi.dylib")  # or .so / .dll

    lib.ce_engine_create.restype  = ctypes.c_void_p
    lib.ce_engine_create.argtypes = [c_float, c_float, c_uint64]

    lib.ce_engine_update.restype  = c_int
    lib.ce_engine_update.argtypes = [ctypes.c_void_p, c_float]

    lib.ce_unit_spawn.restype  = c_int
    lib.ce_unit_spawn.argtypes = [ctypes.c_void_p, c_int, c_float, c_float]

    # Create engine
    engine = lib.ce_engine_create(1600.0, 900.0, 0)

    lib.ce_timer_grant_charge.restype = ctypes.c_int
    lib.ce_timer_grant_charge.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.ce_last_result.restype = ctypes.c_int
    lib.ce_last_result.argtypes = [ctypes.c_void_p]

    # Tick until charge (or grant in tests)
    while not lib.ce_timer_has_charge(engine, 1):
        lib.ce_engine_update(engine, 1.0 / 30.0)

    # Spawn
    lib.ce_unit_spawn(engine, 1, 400.0, 450.0)

    # JSON snapshot
    buf = ctypes.create_string_buffer(65536)
    lib.ce_snapshot_get_json(engine, buf, 65536)
    print(buf.value.decode("utf-8"))

    # Cleanup
    lib.ce_engine_destroy(engine)
"""

# Type stubs for IDE support (implement with ctypes or cffi as above)

def ce_engine_create(mapWidth: float, mapHeight: float, rngSeed: int) -> int: ...
def ce_engine_destroy(engine: int) -> None: ...
def ce_engine_update(engine: int, deltaTime: float) -> int: ...
def ce_engine_reset(engine: int) -> None: ...
def ce_unit_spawn(engine: int, teamId: int, x: float, y: float) -> int: ...
def ce_unit_count(engine: int) -> int: ...
def ce_unit_set_behavior(engine: int, index: int, behaviorName: str) -> int: ...
def ce_field_query(engine: int, x: float, y: float) -> float: ...
def ce_snapshot_get_json(engine: int, buffer: bytes, bufferSize: int) -> int: ...
def ce_timer_has_charge(engine: int, teamId: int) -> int: ...
def ce_timer_get_progress(engine: int, teamId: int) -> float: ...
def ce_timer_grant_charge(engine: int, teamId: int) -> int: ...
def ce_last_result(engine: int) -> int: ...
def ce_territory_get_pct(engine: int, teamId: int) -> float: ...
def ce_version() -> str: ...
def ce_ffi_version() -> str: ...
