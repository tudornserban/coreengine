#!/usr/bin/env bash
# Build and run core.math unit tests without compiling other core/ implementation units.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${COREENGINE_STANDALONE_BUILD_DIR:-${ROOT}/build-math}"
export CXX="${CXX:-g++}"

echo "[run-math-module-tests] fresh CMAKE_BINARY_DIR=${BUILD_DIR}"
rm -rf "${BUILD_DIR}"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DBUILD_TESTS=ON \
  -DCOREENGINE_MATH_STANDALONE=ON

cmake --build "${BUILD_DIR}" --target coreengine_math_tests
cd "${BUILD_DIR}"
ctest --output-on-failure -R 'DeterministicRNGTest|MathConstantsTest|Vector2Test|Vector3Test'
