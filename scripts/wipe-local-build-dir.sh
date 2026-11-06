#!/usr/bin/env bash
# Remove one repo-local build-* tree after scripts/build_dir_guard.py validation.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $# -ne 1 ]]; then
  echo "usage: wipe-local-build-dir.sh <build-dir-name-or-path>" >&2
  exit 2
fi
python3 "${ROOT}/scripts/build_dir_guard.py" "${ROOT}" "$1"