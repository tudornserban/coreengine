#!/usr/bin/env bash
# Regression: BUILD_FFI=ON must produce a loadable libcoreengine_ffi shared library.
# Fails if static/OBJECT inputs were compiled without POSITION_INDEPENDENT_CODE.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-verify-ffi"
export CXX="${CXX:-g++}"

echo "[verify-ffi-build] CMAKE_BINARY_DIR=${BUILD_DIR}"

rm -rf "${BUILD_DIR}"

if command -v ninja >/dev/null 2>&1; then
  CMAKE_GEN="Ninja"
else
  CMAKE_GEN="Unix Makefiles"
fi

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G "${CMAKE_GEN}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DBUILD_FFI=ON \
  -DBUILD_TESTS=OFF

cmake --build "${BUILD_DIR}" --target coreengine_ffi

LIB="${BUILD_DIR}/libcoreengine_ffi.so"
if [[ ! -f "${LIB}" ]]; then
  echo "verify-ffi-build: missing ${LIB}" >&2
  exit 1
fi

python3 - <<PY
import ctypes
import sys
from pathlib import Path

lib_path = Path("${LIB}").resolve()
print(f"verify-ffi-build: loading {lib_path}")
lib = ctypes.CDLL(str(lib_path))

lib.ce_engine_create.restype = ctypes.c_void_p
lib.ce_engine_create.argtypes = (
    ctypes.c_float,
    ctypes.c_float,
    ctypes.c_uint64,
)
lib.ce_engine_destroy.argtypes = (ctypes.c_void_p,)

lib.ce_unit_spawn.restype = ctypes.c_int
lib.ce_unit_spawn.argtypes = (
    ctypes.c_void_p,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.c_float,
)

lib.ce_timer_grant_charge.restype = ctypes.c_int
lib.ce_timer_grant_charge.argtypes = (ctypes.c_void_p, ctypes.c_int)

lib.ce_engine_get_seed.restype = ctypes.c_uint64
lib.ce_engine_get_seed.argtypes = (ctypes.c_void_p,)

lib.ce_engine_reset.argtypes = (ctypes.c_void_p,)

lib.ce_behavior_register_accel.restype = ctypes.c_int
AccelFn = ctypes.CFUNCTYPE(
    None,
    ctypes.c_float,
    ctypes.c_float,
    ctypes.c_float,
    ctypes.c_float,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.c_void_p,
)
lib.ce_behavior_register_accel.argtypes = (
    ctypes.c_char_p,
    AccelFn,
    ctypes.c_void_p,
)

def noop_accel(x, y, vx, vy, team, dt, out_ax, out_ay, userdata):
    out_ax[0] = 0.0
    out_ay[0] = 0.0

if lib.ce_behavior_register_accel(b"VerifyAccel", AccelFn(noop_accel), None) != 1:
    print("verify-ffi-build: ce_behavior_register_accel failed", file=sys.stderr)
    sys.exit(1)

engine = lib.ce_engine_create(1600.0, 900.0, 42)
if not engine:
    print("verify-ffi-build: ce_engine_create returned NULL", file=sys.stderr)
    sys.exit(1)

if lib.ce_unit_spawn(engine, 1, 400.0, 450.0) != 0:
    print("verify-ffi-build: expected ce_unit_spawn 0 without charge", file=sys.stderr)
    sys.exit(1)
if lib.ce_timer_grant_charge(engine, 1) != 1:
    print("verify-ffi-build: ce_timer_grant_charge failed", file=sys.stderr)
    sys.exit(1)

if lib.ce_engine_get_seed(engine) != 42:
    print("verify-ffi-build: ce_engine_get_seed mismatch", file=sys.stderr)
    sys.exit(1)

lib.ce_engine_reset(engine)
if lib.ce_engine_get_seed(engine) != 42:
    print("verify-ffi-build: ce_engine_get_seed after reset mismatch", file=sys.stderr)
    sys.exit(1)

lib.ce_engine_destroy(engine)
print(f"verify-ffi-build: OK loaded {lib_path}")
PY
