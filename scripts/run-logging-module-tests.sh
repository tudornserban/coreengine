#!/usr/bin/env bash
# Build and run core.logging tests (logging object lib only).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-logging"
export CXX="${CXX:-g++}"

echo "[run-logging-module-tests] CMAKE_BINARY_DIR=${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_LOGGING_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_logging_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
