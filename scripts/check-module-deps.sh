#!/usr/bin/env bash
# Validate module manifests; enforce #include boundaries for opted-in modules (Phase 2–3).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[check-module-deps] CoreEngine module manifest + include check"
python3 "${ROOT}/scripts/test_build_dir_guard.py"
python3 "${ROOT}/scripts/check_module_deps.py"
echo "[check-module-deps] done"
