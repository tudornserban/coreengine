#!/usr/bin/env bash
# Combat-only build: core.math + core.interfaces (headers) + core.combat + Combat gtests (no full coreengine monolith).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-combat"
export CXX="${CXX:-g++}"

echo "[run-combat-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DBUILD_FFI=OFF \
  -DCOREENGINE_COMBAT_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_combat_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
