#!/usr/bin/env bash
# Build and run core.gameloop tests (gameloop object lib + declared deps only).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-gameloop"
export CXX="${CXX:-g++}"

echo "[run-gameloop-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_GAMELOOP_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_gameloop_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure