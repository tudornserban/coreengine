#!/usr/bin/env python3
"""Validate CoreEngine module manifests and optional #include boundary enforcement."""
from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

# Committed manifest-only dependency map (requires edges); bumped when graph.json shape changes.
DEPENDENCY_GRAPH_SCHEMA_VERSION = 1
DEPENDENCY_GRAPH_REL = Path("docs/deps/graph.json")
DEPENDENCY_SUMMARY_REL = Path("docs/deps/SUMMARY.md")

# Layer labels align with docs/agentic/DEPENDENCY_POLICY.md (section "Layers").
_MODULE_LAYERS: dict[str, str] = {
    "core.math": "Foundation",
    "core.influence.types": "Foundation",
    "core.logging": "Foundation",
    "core.interfaces": "Contracts",
    "core.ai": "Systems",
    "core.influence": "Systems",
    "core.collision": "Systems",
    "core.map": "Systems",
    "core.gameloop": "Systems",
    "core.combat": "Systems",
    "core.gamestate": "Integration",
    "core.serialization": "Integration",
    "core.ai_advanced": "Experimental",
}

_LAYER_ORDER = ("Foundation", "Contracts", "Systems", "Integration", "Experimental")

SEMVER_RE = re.compile(
    r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"
)
ID_RE = re.compile(r"^[a-z][a-z0-9]*(\.[a-z][a-z0-9_]*)+$")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]', re.MULTILINE)

# Phase 2: strict include boundaries. Other modules remain manifest-only until opted in.
INCLUDE_ENFORCED_MODULES = frozenset(
    {
        "core.math",
        "core.influence.types",
        "core.influence",
        "core.logging",
        "core.interfaces",
        "core.gameloop",
    }
)


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


def module_layer(module_id: str) -> str:
    """Policy layer label from DEPENDENCY_POLICY.md, or Unlabeled for unknown ids."""
    return _MODULE_LAYERS.get(module_id, "Unlabeled")


def build_dependency_graph(
    module_ids: list[str],
    manifests: dict[str, dict],
    enforced_modules: frozenset[str],
) -> dict:
    """Deterministic manifest-only requires graph (sorted modules and edges)."""
    sorted_ids = sorted(module_ids)
    modules: list[dict] = []
    for mid in sorted_ids:
        data = manifests[mid]
        requires = data.get("requires", {}) or {}
        req_obj = {dep: requires[dep] for dep in sorted(requires.keys())}
        modules.append(
            {
                "id": mid,
                "version": data.get("version", ""),
                "path": data.get("path", ""),
                "requires": req_obj,
                "includeEnforced": mid in enforced_modules,
            }
        )
    edges: list[dict] = []
    for mid in sorted_ids:
        requires = manifests[mid].get("requires", {}) or {}
        for dep in sorted(requires.keys()):
            edges.append(
                {
                    "from": mid,
                    "to": dep,
                    "range": requires[dep],
                    "enforced": mid in enforced_modules,
                }
            )
    edges.sort(key=lambda e: (e["from"], e["to"]))
    return {
        "schemaVersion": DEPENDENCY_GRAPH_SCHEMA_VERSION,
        "modules": modules,
        "edges": edges,
    }


def render_dependency_graph_json(graph: dict) -> str:
    """Canonical JSON text for graph.json (trailing newline)."""
    return json.dumps(graph, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def render_dependency_summary_md(
    graph: dict,
    module_ids: list[str],
    manifests: dict[str, dict],
) -> str:
    """Human summary derived from the same structure as graph.json (deterministic)."""
    include_by_id = {m["id"]: m["includeEnforced"] for m in graph["modules"]}
    lines: list[str] = [
        "# Module dependency map (manifest `requires` only)",
        "",
        "Committed artifacts under `docs/deps/` are generated by "
        "`scripts/check_module_deps.py` from `docs/agentic/modules/registry.json` "
        "and each `*.module.json`. Regenerate by running the checker after manifest "
        "edits; do not hand-edit `graph.json` or `SUMMARY.md`.",
        "",
        "## Modules by policy layer",
        "",
    ]
    sorted_ids = sorted(module_ids)
    by_layer: dict[str, list[str]] = {}
    for layer in _LAYER_ORDER:
        by_layer[layer] = []
    by_layer["Unlabeled"] = []
    for mid in sorted_ids:
        layer = module_layer(mid)
        if layer not in by_layer:
            by_layer[layer] = []
        by_layer[layer].append(mid)
    extra_layers = sorted(k for k in by_layer if k not in _LAYER_ORDER and k != "Unlabeled")
    section_order = list(_LAYER_ORDER) + extra_layers + ["Unlabeled"]
    for layer in section_order:
        mods = sorted(by_layer.get(layer, []))
        if not mods:
            continue
        lines.append(f"### {layer}")
        lines.append("")
        for mid in mods:
            data = manifests[mid]
            req = data.get("requires", {}) or {}
            ie = "yes" if include_by_id[mid] else "no"
            req_bits = ", ".join(f"`{d}` {req[d]!r}" for d in sorted(req.keys()))
            if not req_bits:
                req_bits = "_(none)_"
            lines.append(
                f"- **{mid}** — `{data['path']}` v{data['version']} — "
                f"requires: {req_bits} — include enforcement: {ie}"
            )
        lines.append("")
    lines.extend(
        [
            "## Requires edges (manifest only)",
            "",
            "Sorted by `from`, then `to`.",
            "",
            "| from | to | range | from includes enforced |",
            "| --- | --- | --- | --- |",
        ]
    )
    for e in graph["edges"]:
        en = "yes" if e["enforced"] else "no"
        lines.append(f"| `{e['from']}` | `{e['to']}` | `{e['range']}` | {en} |")
    lines.append("")
    return "\n".join(lines) + "\n"


def verify_core_interfaces_changelog(repo_root: Path, manifests: dict[str, dict]) -> list[str]:
    """Ensure core/interfaces/CHANGELOG.md documents the manifest version for core.interfaces."""
    errors: list[str] = []
    mid = "core.interfaces"
    if mid not in manifests:
        return errors
    version = manifests[mid].get("version", "")
    if not version or not SEMVER_RE.match(version):
        return errors
    changelog_path = repo_root / "core" / "interfaces" / "CHANGELOG.md"
    if not changelog_path.is_file():
        errors.append(
            "core/interfaces/CHANGELOG.md: missing; add it when maintaining core.interfaces "
            "(semver section must match docs/agentic/modules/core.interfaces.module.json)"
        )
        return errors
    text = changelog_path.read_text(encoding="utf-8", errors="replace")
    heading = f"## [{version}]"
    if heading not in text:
        errors.append(
            f"core/interfaces/CHANGELOG.md: expected a line {heading!r} matching manifest version "
            f"{version!r}; append a changelog entry when bumping core.interfaces"
        )
    return errors


def verify_dependency_map_artifacts(
    repo_root: Path,
    module_ids: list[str],
    manifests: dict[str, dict],
    enforced_modules: frozenset[str],
) -> list[str]:
    """Return errors if committed graph.json / SUMMARY.md differ from the generator (bytes)."""
    errors: list[str] = []
    if len(module_ids) != len(manifests):
        return errors
    if any(mid not in manifests for mid in module_ids):
        return errors
    graph = build_dependency_graph(module_ids, manifests, enforced_modules)
    expected_json = render_dependency_graph_json(graph)
    expected_md = render_dependency_summary_md(graph, module_ids, manifests)

    graph_path = repo_root / DEPENDENCY_GRAPH_REL
    summary_path = repo_root / DEPENDENCY_SUMMARY_REL
    if not graph_path.is_file():
        errors.append(f"missing committed dependency map: {DEPENDENCY_GRAPH_REL}")
    else:
        actual = graph_path.read_bytes()
        if actual != expected_json.encode("utf-8"):
            errors.append(
                f"{DEPENDENCY_GRAPH_REL}: out of date or hand-edited; "
                f"regenerate from manifests (byte mismatch with generator output)"
            )
    if not summary_path.is_file():
        errors.append(f"missing committed dependency summary: {DEPENDENCY_SUMMARY_REL}")
    else:
        actual = summary_path.read_bytes()
        if actual != expected_md.encode("utf-8"):
            errors.append(
                f"{DEPENDENCY_SUMMARY_REL}: out of date or hand-edited; "
                f"regenerate from manifests (byte mismatch with generator output)"
            )
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
    # Drift gate: committed docs/deps/* must match manifest-only generator output (byte-for-byte).
    if not manifest_errors and len(module_ids) == len(manifests) and all(
        m in manifests for m in module_ids
    ):
        errors.extend(verify_dependency_map_artifacts(repo_root, module_ids, manifests, enforced))

    errors.extend(verify_core_interfaces_changelog(repo_root, manifests))

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
    print(f"OK: dependency map matches committed {DEPENDENCY_GRAPH_REL} and {DEPENDENCY_SUMMARY_REL}.")
    print(f"OK: include boundaries enforced for: {enforced}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
