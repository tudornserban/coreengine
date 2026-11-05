#!/usr/bin/env bash
# Phase 1 stub: validate module manifests and registry consistency.
# Future: parse #includes under each module path and enforce requires + illegal-import rules.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REGISTRY="${ROOT}/docs/agentic/modules/registry.json"
MODULES_DIR="${ROOT}/docs/agentic/modules"
SCHEMA="${ROOT}/docs/agentic/manifest.schema.json"

echo "[check-module-deps] CoreEngine module manifest check (stub)"

if [[ ! -f "${REGISTRY}" ]]; then
  echo "ERROR: missing registry ${REGISTRY}" >&2
  exit 1
fi

python3 - "${REGISTRY}" "${MODULES_DIR}" "${SCHEMA}" <<'PY'
import json
import re
import sys
from pathlib import Path

registry_path = Path(sys.argv[1])
modules_dir = Path(sys.argv[2])
schema_path = Path(sys.argv[3])

semver_re = re.compile(
    r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"
)
id_re = re.compile(r"^[a-z][a-z0-9]*(\.[a-z][a-z0-9_]*)+$")

registry = json.loads(registry_path.read_text())
module_ids = registry.get("modules", [])
errors = []

manifests = {}
for mid in module_ids:
    manifest_file = modules_dir / f"{mid}.module.json"
    if not manifest_file.is_file():
        errors.append(f"missing manifest for {mid}: {manifest_file}")
        continue
    data = json.loads(manifest_file.read_text())
    manifests[mid] = data

    if data.get("id") != mid:
        errors.append(f"{manifest_file}: id {data.get('id')!r} != registry entry {mid!r}")
    if not id_re.match(data.get("id", "")):
        errors.append(f"{manifest_file}: invalid id")
    if not semver_re.match(data.get("version", "")):
        errors.append(f"{manifest_file}: invalid version {data.get('version')!r}")
    path = data.get("path", "")
    repo_root = registry_path.parents[3]
    if not (repo_root / path).is_dir():
        errors.append(f"{manifest_file}: path not found: {path}")

    for dep in data.get("requires", {}):
        if dep not in module_ids:
            errors.append(f"{manifest_file}: requires unknown module {dep!r}")

extra = {p.stem.replace(".module", "") for p in modules_dir.glob("*.module.json")}
extra -= set(module_ids)
if extra:
    errors.append(f"manifests not listed in registry: {sorted(extra)}")

# Stub: cycle detection on requires graph only
WHITE, GRAY, BLACK = 0, 1, 2
color = {m: WHITE for m in module_ids}

def dfs(node, stack):
    color[node] = GRAY
    stack.append(node)
    req = manifests.get(node, {}).get("requires", {})
    for dep in req:
        if dep not in color:
            continue
        if color[dep] == GRAY:
            errors.append(f"cycle in requires graph involving {dep}")
            return
        if color[dep] == WHITE:
            dfs(dep, stack)
    stack.pop()
    color[node] = BLACK

for m in module_ids:
    if color[m] == WHITE:
        dfs(m, [])

if errors:
    print("FAIL:")
    for e in errors:
        print(f"  - {e}")
    sys.exit(1)

print(f"OK: {len(module_ids)} module manifests validated.")
print("NOTE: include-path enforcement not implemented yet (Phase 2).")
PY

echo "[check-module-deps] done"
