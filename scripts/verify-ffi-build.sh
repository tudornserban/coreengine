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

engine = lib.ce_engine_create(1600.0, 900.0, 42)
if not engine:
    print("verify-ffi-build: ce_engine_create returned NULL", file=sys.stderr)
    sys.exit(1)
lib.ce_engine_destroy(engine)
print(f"verify-ffi-build: OK loaded {lib_path}")
PY