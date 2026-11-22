#!/usr/bin/env bash
set -euo pipefail

# Packages and installs the CoreEngineUE5 plugin (including core/ and platform/unreal/
# C++17 sources) into an Unreal Engine 5 project directory.
#
# Usage:
#   ./scripts/install-ue5-plugin.sh /path/to/YourUE5Project

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <path-to-ue5-project-directory>"
    echo "Example: $0 ~/Documents/Unreal\\ Projects/MyGame"
    exit 1
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_PROJECT_DIR="$1"

if [[ ! -d "${TARGET_PROJECT_DIR}" ]]; then
    echo "Error: Target directory does not exist: ${TARGET_PROJECT_DIR}"
    exit 1
fi

PLUGIN_DEST="${TARGET_PROJECT_DIR}/Plugins/CoreEngineUE5"
THIRDPARTY_DEST="${PLUGIN_DEST}/ThirdParty/CoreEngine"
GENERATED_SRC_DEST="${PLUGIN_DEST}/Source/CoreEngineUE5/Private/GeneratedCoreSources"

echo "[install-ue5-plugin] Installing CoreEngineUE5 into: ${PLUGIN_DEST}"
rm -rf "${PLUGIN_DEST}"
mkdir -p "${PLUGIN_DEST}"
mkdir -p "${THIRDPARTY_DEST}"
mkdir -p "${GENERATED_SRC_DEST}"

# 1. Copy the UE5 plugin descriptor and module sources
cp -R "${REPO_ROOT}/platform/unreal/CoreEngineUE5/"* "${PLUGIN_DEST}/"

# 2. Bundle core/ and platform/ headers & sources inside ThirdParty/CoreEngine
cp -R "${REPO_ROOT}/core" "${THIRDPARTY_DEST}/"
mkdir -p "${THIRDPARTY_DEST}/platform"
cp -R "${REPO_ROOT}/platform/unreal" "${THIRDPARTY_DEST}/platform/"
cp -R "${REPO_ROOT}/platform/host" "${THIRDPARTY_DEST}/platform/"
cp -R "${REPO_ROOT}/platform/null" "${THIRDPARTY_DEST}/platform/"

# Remove nested plugin copy inside ThirdParty/CoreEngine/platform/unreal/CoreEngineUE5
rm -rf "${THIRDPARTY_DEST}/platform/unreal/CoreEngineUE5"

# 3. Generate per-file translation unit wrappers under Source/CoreEngineUE5/Private/GeneratedCoreSources
# so UnrealBuildTool compiles every CoreEngine and UE5 bridge .cpp file automatically.
idx=0
while IFS= read -r -d '' cpp_file; do
    rel_path="${cpp_file#${THIRDPARTY_DEST}/}"
    base_name="$(basename "${cpp_file}" .cpp)"
    idx=$((idx + 1))
    wrapper_file="${GENERATED_SRC_DEST}/CE_${idx}_${base_name}.cpp"
    cat > "${wrapper_file}" <<EOF
// Auto-generated UnrealBuildTool translation unit wrapper for ${rel_path}
#pragma push_macro("PI")
#undef PI
#pragma push_macro("HALF_PI")
#undef HALF_PI
#pragma push_macro("TWO_PI")
#undef TWO_PI
#pragma push_macro("check")
#undef check
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4458 4100 4189)
#endif
#include "${rel_path}"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma pop_macro("check")
#pragma pop_macro("TWO_PI")
#pragma pop_macro("HALF_PI")
#pragma pop_macro("PI")
EOF
done < <(find "${THIRDPARTY_DEST}/core" "${THIRDPARTY_DEST}/platform/unreal" -name "*.cpp" -print0 | sort -z)

echo "[install-ue5-plugin] Installed ${idx} CoreEngine C++17 translation units into CoreEngineUE5."
echo ""
echo "Next steps in Unreal Engine 5:"
echo "  1. Open your .uproject in Unreal Engine 5 (click 'Yes' when prompted to rebuild CoreEngineUE5)."
echo "  2. In the Unreal Editor, open Window -> World Settings."
echo "  3. Set 'GameMode Override' to 'CoreEngineVisualTestGameMode' (or drag 'CoreEngineVisualTestActor' from Place Actors into the level)."
echo "  4. Press Play (Alt+P / Cmd+P) to run the 3D Placeholder Sandbox, 7-panel GUI, and Tactical Minimap!"
