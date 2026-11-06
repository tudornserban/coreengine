#!/usr/bin/env bash
# Collision-only build: core.math + core.interfaces (headers) + core.logging + core.collision + Collision gtests (no full coreengine monolith).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-collision"
export CXX="${CXX:-g++}"

echo "[run-collision-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DBUILD_FFI=OFF \
  -DCOREENGINE_COLLISION_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_collision_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
