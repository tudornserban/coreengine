#!/usr/bin/env bash
# Build and run core.interfaces header compile tests (math + influence.types object libs only).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-interfaces"
export CXX="${CXX:-g++}"

echo "[run-interfaces-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_INTERFACES_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_interfaces_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure