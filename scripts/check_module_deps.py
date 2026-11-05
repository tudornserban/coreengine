#!/usr/bin/env python3
"""Validate CoreEngine module manifests and optional #include boundary enforcement."""
from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SEMVER_RE = re.compile(
    r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"
)
ID_RE = re.compile(r"^[a-z][a-z0-9]*(\.[a-z][a-z0-9_]*)+$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]', re.MULTILINE)

# Phase 2: strict include boundaries. Other modules remain manifest-only until opted in.
INCLUDE_ENFORCED_MODULES = frozenset({"core.math", "core.influence.types", "core.influence"})


@dataclass
class CheckResult:
    errors: list[str]
    warnings: list[str]

    @property
    def ok(self) -> bool:
        return not self.errors


def load_manifests(registry_path: Path, modules_dir: Path) -> tuple[list[str], dict, list[str]]:
    errors: list[str] = []
    registry = json.loads(registry_path.read_text())
    module_ids: list[str] = registry.get("modules", [])
    manifests: dict[str, dict] = {}

    for mid in module_ids:
        manifest_file = modules_dir / f"{mid}.module.json"
        if not manifest_file.is_file():
            errors.append(f"missing manifest for {mid}: {manifest_file}")
            continue
        data = json.loads(manifest_file.read_text())
        manifests[mid] = data

        if data.get("id") != mid:
            errors.append(f"{manifest_file}: id {data.get('id')!r} != registry entry {mid!r}")
        if not ID_RE.match(data.get("id", "")):
            errors.append(f"{manifest_file}: invalid id")
        if not SEMVER_RE.match(data.get("version", "")):
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

    return module_ids, manifests, errors


def detect_requires_cycles(module_ids: list[str], manifests: dict) -> list[str]:
    errors: list[str] = []
    WHITE, GRAY, BLACK = 0, 1, 2
    color = {m: WHITE for m in module_ids}

    def dfs(node: str, stack: list[str]) -> None:
        color[node] = GRAY
        stack.append(node)
        req = manifests.get(node, {}).get("requires", {})
        for dep in req:
            if dep not in color:
                continue
            if color[dep] == GRAY:
                errors.append(f"cycle in requires graph involving {dep}")
                continue
            if color[dep] == WHITE:
                dfs(dep, stack)
        stack.pop()
        color[node] = BLACK

    for m in module_ids:
        if color[m] == WHITE:
            dfs(m, [])
    return errors


def module_id_for_core_include(include_path: str, manifests: dict) -> str | None:
    if not include_path.startswith("core/"):
        return None
    best: tuple[int, str | None] = (0, None)
    for mid, data in manifests.items():
        mod_path = data.get("path", "").rstrip("/")
        if include_path == mod_path or include_path.startswith(mod_path + "/"):
            if len(mod_path) > best[0]:
                best = (len(mod_path), mid)
    return best[1]


def check_include_line(
    include_path: str,
    *,
    source_file: Path,
    repo_root: Path,
    module_id: str,
    module_root: Path,
    manifests: dict,
) -> str | None:
    """Return an error message if the include violates module boundaries, else None."""
    requires = manifests[module_id].get("requires", {})

    if include_path.startswith("core/"):
        target = module_id_for_core_include(include_path, manifests)
        if target is None:
            return f"unknown core include {include_path!r}"
        if target == module_id:
            return None
        if target not in requires:
            return (
                f"include {include_path!r} pulls in {target!r} "
                f"but manifest requires does not list it"
            )
        return None

    if include_path.startswith("platform/") or include_path.startswith("ffi/"):
        return f"illegal include {include_path!r} (core modules must not include platform/ffi)"

    resolved = (source_file.parent / include_path).resolve()
    try:
        resolved.relative_to(module_root.resolve())
        return None
    except ValueError:
        pass

    for req_id in requires:
        req_path = (repo_root / manifests[req_id].get("path", "")).resolve()
        if not req_path.is_dir():
            continue
        try:
            resolved.relative_to(req_path)
            return None
        except ValueError:
            continue

    return f"include {include_path!r} resolves outside {module_root.relative_to(repo_root)}"


def check_include_boundaries(
    repo_root: Path,
    module_ids: list[str],
    manifests: dict,
    enforced: frozenset[str] = INCLUDE_ENFORCED_MODULES,
) -> list[str]:
    errors: list[str] = []
    for mid in module_ids:
        if mid not in enforced:
            continue
        mod_path = manifests[mid].get("path", "")
        module_root = repo_root / mod_path
        if not module_root.is_dir():
            continue
        for source in sorted(module_root.rglob("*")):
            if source.suffix not in {".h", ".hpp", ".cpp", ".cc"}:
                continue
            text = source.read_text(encoding="utf-8", errors="replace")
            for match in INCLUDE_RE.finditer(text):
                include_path = match.group(2)
                err = check_include_line(
                    include_path,
                    source_file=source,
                    repo_root=repo_root,
                    module_id=mid,
                    module_root=module_root,
                    manifests=manifests,
                )
                if err:
                    rel = source.relative_to(repo_root)
                    errors.append(f"{mid}: {rel}: {err}")
    return errors


def run_checks(
    repo_root: Path,
    *,
    enforced_modules: frozenset[str] | None = None,
) -> CheckResult:
    registry_path = repo_root / "docs/agentic/modules/registry.json"
    modules_dir = repo_root / "docs/agentic/modules"
    errors: list[str] = []
    warnings: list[str] = []

    module_ids, manifests, manifest_errors = load_manifests(registry_path, modules_dir)
    errors.extend(manifest_errors)
    errors.extend(detect_requires_cycles(module_ids, manifests))

    enforced = enforced_modules if enforced_modules is not None else INCLUDE_ENFORCED_MODULES
    errors.extend(check_include_boundaries(repo_root, module_ids, manifests, enforced))

    skipped = set(module_ids) - set(enforced)
    if skipped:
        warnings.append(
            "include-path enforcement skipped for: "
            + ", ".join(sorted(skipped))
            + " (manifest validation only)"
        )

    return CheckResult(errors=errors, warnings=warnings)


def main(argv: list[str] | None = None) -> int:
    repo_root = Path(__file__).resolve().parents[1]
    result = run_checks(repo_root)
    for w in result.warnings:
        print(f"NOTE: {w}")
    if not result.ok:
        print("FAIL:")
        for e in result.errors:
            print(f"  - {e}")
        return 1
    n = len(json.loads((repo_root / "docs/agentic/modules/registry.json").read_text())["modules"])
    enforced = ", ".join(sorted(INCLUDE_ENFORCED_MODULES))
    print(f"OK: {n} module manifests validated.")
    print(f"OK: include boundaries enforced for: {enforced}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
