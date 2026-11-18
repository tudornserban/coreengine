#!/usr/bin/env bash
# Configure and build the CoreEngine static library and C FFI shared library
# for one host platform: linux, macos, windows, or android.
#
# Desktop tests (BUILD_TESTS, COREENGINE_*_STANDALONE) stay on the native Linux
# gate. This script produces the libraries a game or Unity plugin loads.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-}"
ABI="${ANDROID_ABI:-arm64-v8a}"
API_LEVEL="${ANDROID_PLATFORM:-android-24}"
ABI_SET=0
API_SET=0

usage() {
  cat >&2 <<EOF
Usage: $0 <linux|macos|windows|android> [--abi ABI] [--platform android-NN]

  linux     native Linux (libcoreengine_ffi.so)
  macos     native Mac, or osxcross when OSXCROSS_ROOT is set (libcoreengine_ffi.dylib)
  windows   MinGW-w64 cross build, or native MinGW (coreengine_ffi.dll)
  android   NDK r25+ via ANDROID_NDK (libcoreengine_ffi.so)

Android defaults: ABI arm64-v8a, API android-24, STL c++_shared.
EOF
  exit 2
}

if [[ -z "${TARGET}" ]]; then
  usage
fi
shift

while [[ $# -gt 0 ]]; do
  case "$1" in
    --abi)
      ABI="${2:-}"
      ABI_SET=1
      shift 2
      ;;
    --platform)
      API_LEVEL="${2:-}"
      API_SET=1
      shift 2
      ;;
    *)
      echo "build-host-platform: unknown argument: $1" >&2
      usage
      ;;
  esac
done

if [[ "${ABI_SET}" == 1 || "${API_SET}" == 1 ]] && [[ "${TARGET}" != "android" ]]; then
  echo "build-host-platform: --abi and --platform are only valid for android" >&2
  exit 2
fi

HOST="$(uname -s)"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

if command -v ninja >/dev/null 2>&1; then
  GENERATOR=( -G Ninja )
else
  GENERATOR=( -G "Unix Makefiles" )
fi

CMAKE_ARGS=(
  -S "${ROOT}"
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  -DBUILD_FFI=ON
  -DBUILD_TESTS=OFF
  -DBUILD_SDL2=OFF
  -DBUILD_RAYLIB=OFF
  -DCOREENGINE_BUILD_SDL2_VIEWER=OFF
)

case "${TARGET}" in
  linux)
    if [[ "${HOST}" != "Linux" ]]; then
      echo "build-host-platform: the linux target is a native build. Run it on Linux." >&2
      exit 1
    fi
    BUILD_DIR="${ROOT}/build-linux"
    CMAKE_ARGS+=( -DCMAKE_CXX_COMPILER="${CXX:-g++}" )
    SHARED_NAME="libcoreengine_ffi.so"
    ;;
  macos)
    BUILD_DIR="${ROOT}/build-macos"
    SHARED_NAME="libcoreengine_ffi.dylib"
    if [[ "${HOST}" == "Darwin" && -z "${OSXCROSS_ROOT:-}" ]]; then
      CMAKE_ARGS+=( -DCMAKE_CXX_COMPILER="${CXX:-c++}" )
    else
      CMAKE_ARGS+=( -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/macos.cmake" )
    fi
    ;;
  windows)
    BUILD_DIR="${ROOT}/build-windows"
    SHARED_NAME="coreengine_ffi.dll"
    case "${HOST}" in
      MINGW*|MSYS*|CYGWIN*)
        CMAKE_ARGS+=( -DCMAKE_CXX_COMPILER="${CXX:-g++}" )
        ;;
      *)
        CMAKE_ARGS+=( -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/windows-mingw.cmake" )
        ;;
    esac
    ;;
  android)
    BUILD_DIR="${ROOT}/build-android-${ABI}"
    SHARED_NAME="libcoreengine_ffi.so"
    CMAKE_ARGS+=(
      -DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/toolchains/android.cmake"
      -DANDROID_ABI="${ABI}"
      -DANDROID_PLATFORM="${API_LEVEL}"
    )
    if [[ -n "${ANDROID_NDK:-}" ]]; then
      CMAKE_ARGS+=( -DANDROID_NDK="${ANDROID_NDK}" )
    elif [[ -n "${ANDROID_NDK_ROOT:-}" ]]; then
      CMAKE_ARGS+=( -DANDROID_NDK="${ANDROID_NDK_ROOT}" )
    elif [[ -n "${ANDROID_NDK_HOME:-}" ]]; then
      CMAKE_ARGS+=( -DANDROID_NDK="${ANDROID_NDK_HOME}" )
    fi
    ;;
  *)
    echo "build-host-platform: unknown target '${TARGET}'" >&2
    usage
    ;;
esac

echo "[build-host-platform] target=${TARGET} build=${BUILD_DIR} type=${BUILD_TYPE}"
cmake "${CMAKE_ARGS[@]}" -B "${BUILD_DIR}" "${GENERATOR[@]}"
cmake --build "${BUILD_DIR}" --parallel --target coreengine_ffi
cmake --build "${BUILD_DIR}" --parallel --target coreengine_host_check

find_optional() {
  local name="$1"
  local found
  found="$(find "${BUILD_DIR}" -type f -name "${name}" ! -path '*/CMakeFiles/*' || true)"
  found="${found%%$'\n'*}"
  printf '%s\n' "${found}"
}

require_file() {
  local label="$1"
  local path="$2"
  if [[ -z "${path}" ]]; then
    echo "build-host-platform: missing ${label} under ${BUILD_DIR}" >&2
    exit 1
  fi
}

SHARED_PATH="$(find_optional "${SHARED_NAME}")"
require_file "${SHARED_NAME}" "${SHARED_PATH}"
STATIC_PATH="$(find_optional "libcoreengine.a")"
if [[ -z "${STATIC_PATH}" ]]; then
  STATIC_PATH="$(find_optional "coreengine.lib")"
fi
require_file "libcoreengine.a or coreengine.lib" "${STATIC_PATH}"
HOST_CHECK="$(find_optional "libcoreengine_host_check.a")"
if [[ -z "${HOST_CHECK}" ]]; then
  HOST_CHECK="$(find_optional "coreengine_host_check.lib")"
fi
require_file "coreengine_host_check" "${HOST_CHECK}"

FFI_EXPORTS=(
  ce_engine_create
  ce_engine_destroy
  ce_engine_update
  ce_engine_reset
  ce_engine_get_seed
  ce_unit_spawn
  ce_unit_count
  ce_unit_get_state
  ce_unit_set_behavior
  ce_field_query
  ce_snapshot_get
  ce_snapshot_get_json
  ce_timer_has_charge
  ce_timer_get_progress
  ce_timer_grant_charge
  ce_last_result
  ce_territory_get_pct
  ce_behavior_register
  ce_behavior_register_accel
  ce_behavior_list
  ce_version
  ce_ffi_version
)

require_exports_in_text() {
  local bin="$1"
  local dump="$2"
  local sym
  for sym in "${FFI_EXPORTS[@]}"; do
    if ! grep -q -E "[[:space:]]${sym}([[:space:]]|$)" <<<"${dump}"; then
      echo "build-host-platform: ${bin} does not export ${sym}" >&2
      exit 1
    fi
  done
}

case "${TARGET}" in
  windows)
    if command -v x86_64-w64-mingw32-objdump >/dev/null 2>&1; then
      DUMP=(x86_64-w64-mingw32-objdump)
    else
      DUMP=(objdump)
    fi
    require_exports_in_text "${SHARED_PATH}" "$("${DUMP[@]}" -p "${SHARED_PATH}")"
    ;;
  linux|android)
    if [[ "${TARGET}" == android ]]; then
      NDK_ROOT="${ANDROID_NDK:-${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-}}}"
      NM_TOOL="${NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm"
      if [[ ! -x "${NM_TOOL}" ]]; then
        NM_TOOL="$(find "${NDK_ROOT}/toolchains/llvm/prebuilt" -type f -name llvm-nm 2>/dev/null || true)"
        NM_TOOL="${NM_TOOL%%$'\n'*}"
      fi
    else
      NM_TOOL="nm"
    fi
    if [[ ! -x "${NM_TOOL}" ]] && ! command -v "${NM_TOOL}" >/dev/null 2>&1; then
      echo "build-host-platform: cannot find ${NM_TOOL} to check exports" >&2
      exit 1
    fi
    require_exports_in_text "${SHARED_PATH}" "$("${NM_TOOL}" -D --defined-only "${SHARED_PATH}")"
    ;;
esac

echo "build-host-platform: ${TARGET} OK"
echo "  static: ${STATIC_PATH}"
echo "  shared: ${SHARED_PATH}"
echo "  host check: ${HOST_CHECK}"
