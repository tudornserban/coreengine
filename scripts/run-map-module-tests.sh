#!/usr/bin/env bash
# Map-only build: core.math + core.influence.types + core.map + Map gtests (no full coreengine monolith).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-map"
export CXX="${CXX:-g++}"

echo "[run-map-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DBUILD_FFI=OFF \
  -DCOREENGINE_MAP_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_map_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure