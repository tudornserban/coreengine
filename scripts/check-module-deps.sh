#!/usr/bin/env bash
# Validate module manifests; enforce #include boundaries for Phase 2 pilot modules.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[check-module-deps] CoreEngine module manifest + include check"
python3 "${ROOT}/scripts/check_module_deps.py"
echo "[check-module-deps] done"
