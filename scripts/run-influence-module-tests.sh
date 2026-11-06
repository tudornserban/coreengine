#!/usr/bin/env bash
# Build and run core.influence unit tests without compiling unrelated core/ implementation units.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-influence"
export CXX="${CXX:-g++}"

echo "[run-influence-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_INFLUENCE_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_influence_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure