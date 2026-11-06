#!/usr/bin/env bash
# Serialization-only build: OBJECT libs + gamestate hub deps + SerializationModuleHeaderCompileTest (no full coreengine monolith).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-serialization"
export CXX="${CXX:-g++}"

echo "[run-serialization-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

if command -v ninja >/dev/null 2>&1; then
  CMAKE_GEN="Ninja"
else
  CMAKE_GEN="Unix Makefiles"
fi

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G "${CMAKE_GEN}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DBUILD_TESTS=ON \
  -DBUILD_FFI=OFF \
  -DCOREENGINE_SERIALIZATION_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_serialization_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
