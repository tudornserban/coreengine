#!/usr/bin/env python3
"""Regression tests for scripts/check_module_deps.py (manifest + core.math includes)."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_module_deps import (
    DEPENDENCY_GRAPH_REL,
    DEPENDENCY_SUMMARY_REL,
    INCLUDE_ENFORCED_MODULES,
    build_dependency_graph,
    check_include_boundaries,
    detect_requires_cycles,
    render_dependency_graph_json,
    render_dependency_summary_md,
    run_checks,
    verify_core_interfaces_changelog,
    verify_dependency_map_artifacts,
)


class CheckModuleDepsRegression(unittest.TestCase):
    def test_real_repo_passes(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        result = run_checks(repo_root)
        self.assertTrue(result.ok, result.errors)

    def test_core_interfaces_changelog_requires_version_heading(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cl = root / "core" / "interfaces" / "CHANGELOG.md"
            cl.parent.mkdir(parents=True)
            cl.write_text("# Log\n\n## [1.0.0]\n\nhello\n", encoding="utf-8")
            manifests = {"core.interfaces": {"version": "1.0.1"}}
            self.assertTrue(verify_core_interfaces_changelog(root, manifests))

    def test_core_interfaces_changelog_passes_when_heading_matches(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            cl = root / "core" / "interfaces" / "CHANGELOG.md"
            cl.parent.mkdir(parents=True)
            cl.write_text("# Log\n\n## [2.0.0]\n\nok\n", encoding="utf-8")
            manifests = {"core.interfaces": {"version": "2.0.0"}}
            self.assertEqual(verify_core_interfaces_changelog(root, manifests), [])

    def test_real_repo_graph_matches_manifests(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        registry = json.loads(
            (repo_root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
        )
        module_ids = registry["modules"]
        manifests: dict = {}
        for mid in module_ids:
            p = repo_root / "docs/agentic/modules" / f"{mid}.module.json"
            manifests[mid] = json.loads(p.read_text(encoding="utf-8"))
        graph = build_dependency_graph(module_ids, manifests, INCLUDE_ENFORCED_MODULES)
        for node in graph["modules"]:
            mid = node["id"]
            self.assertEqual(node["version"], manifests[mid]["version"])
            self.assertEqual(node["path"], manifests[mid]["path"])
            self.assertEqual(
                node["requires"],
                {d: manifests[mid]["requires"][d] for d in sorted(manifests[mid].get("requires", {}))},
            )
            self.assertEqual(node["includeEnforced"], mid in INCLUDE_ENFORCED_MODULES)
        req_edges = set()
        for mid in module_ids:
            for dep, rng in (manifests[mid].get("requires") or {}).items():
                req_edges.add((mid, dep, rng))
        file_edges = {(e["from"], e["to"], e["range"]) for e in graph["edges"]}
        self.assertEqual(file_edges, req_edges)

    def test_committed_deps_match_generator(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        registry = json.loads(
            (repo_root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
        )
        module_ids = registry["modules"]
        manifests: dict = {}
        for mid in module_ids:
            p = repo_root / "docs/agentic/modules" / f"{mid}.module.json"
            manifests[mid] = json.loads(p.read_text(encoding="utf-8"))
        graph = build_dependency_graph(module_ids, manifests, INCLUDE_ENFORCED_MODULES)
        expected_json = render_dependency_graph_json(graph)
        expected_md = render_dependency_summary_md(graph, module_ids, manifests)
        self.assertEqual((repo_root / DEPENDENCY_GRAPH_REL).read_text(encoding="utf-8"), expected_json)
        self.assertEqual((repo_root / DEPENDENCY_SUMMARY_REL).read_text(encoding="utf-8"), expected_md)
        self.assertEqual(
            verify_dependency_map_artifacts(repo_root, module_ids, manifests, INCLUDE_ENFORCED_MODULES),
            [],
        )

    def test_manifest_edit_changes_graph(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_two_module_registry(root, extra_require=False)
            registry = json.loads(
                (root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
            )
            module_ids = registry["modules"]
            manifests = self._manifests(root)
            g1 = build_dependency_graph(module_ids, manifests, frozenset())
            # Add core.b -> core.a require on core.b only (DAG: a has no deps, b requires a).
            mod_path = root / "docs/agentic/modules/core.b.module.json"
            data = json.loads(mod_path.read_text(encoding="utf-8"))
            data["requires"] = {"core.a": "^1.0.0"}
            mod_path.write_text(json.dumps(data), encoding="utf-8")
            manifests2 = self._manifests(root)
            g2 = build_dependency_graph(module_ids, manifests2, frozenset())
            self.assertEqual(len(g1["edges"]) + 1, len(g2["edges"]))
            self.assertIn(
                {"from": "core.b", "to": "core.a", "range": "^1.0.0", "enforced": False},
                g2["edges"],
            )

    def test_dependency_map_drift_fails_checker(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_min_registry(root)
            (root / "core/math").mkdir(parents=True, exist_ok=True)
            (root / "core/logging").mkdir(parents=True, exist_ok=True)
            registry = json.loads(
                (root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
            )
            module_ids = registry["modules"]
            manifests = self._manifests(root)
            graph = build_dependency_graph(module_ids, manifests, INCLUDE_ENFORCED_MODULES)
            deps = root / "docs/deps"
            deps.mkdir(parents=True, exist_ok=True)
            (deps / "graph.json").write_text(render_dependency_graph_json(graph), encoding="utf-8")
            (deps / "SUMMARY.md").write_text(
                render_dependency_summary_md(graph, module_ids, manifests), encoding="utf-8"
            )
            self.assertTrue(run_checks(root).ok)
            (deps / "graph.json").write_text(
                render_dependency_graph_json(graph).replace('"schemaVersion": 1', '"schemaVersion": 99'),
                encoding="utf-8",
            )
            r = run_checks(root)
            self.assertFalse(r.ok)
            self.assertTrue(any("graph.json" in e for e in r.errors), r.errors)

    def test_run_checks_fails_on_requires_cycle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_cyclic_registry(root)
            (root / "core/interfaces").mkdir(parents=True, exist_ok=True)
            (root / "core/influence").mkdir(parents=True, exist_ok=True)
            deps = root / "docs/deps"
            deps.mkdir(parents=True, exist_ok=True)
            # Valid committed map matching manifests (still cyclic requires).
            registry = json.loads(
                (root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
            )
            module_ids = registry["modules"]
            manifests = self._manifests(root)
            graph = build_dependency_graph(module_ids, manifests, INCLUDE_ENFORCED_MODULES)
            (deps / "graph.json").write_text(render_dependency_graph_json(graph), encoding="utf-8")
            (deps / "SUMMARY.md").write_text(
                render_dependency_summary_md(graph, module_ids, manifests), encoding="utf-8"
            )
            result = run_checks(root)
            self.assertFalse(result.ok)
            self.assertTrue(any("cycle" in e for e in result.errors), result.errors)

    def test_ibehavior_includes_influence_types_leaf(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        text = (repo_root / "core/interfaces/IBehavior.h").read_text(encoding="utf-8")
        self.assertIn("influence_types/InfluenceFieldItem.h", text)
        self.assertNotIn("influence/InfluenceFieldItem.h", text)

    def test_math_rejects_sibling_core_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_min_registry(root)
            math_dir = root / "core/math"
            math_dir.mkdir(parents=True)
            (math_dir / "Bad.cpp").write_text(
                '#include "core/logging/Logger.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            module_ids = list(manifests.keys())
            errors = check_include_boundaries(root, module_ids, manifests, INCLUDE_ENFORCED_MODULES)
            self.assertTrue(any("core/logging" in e for e in errors), errors)

    def test_math_allows_stdlib_and_own_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_min_registry(root)
            math_dir = root / "core/math"
            math_dir.mkdir(parents=True)
            (math_dir / "Ok.cpp").write_text(
                '#include "DeterministicRNG.h"\n#include <cmath>\n#include "core/math/Vector2.h"\n',
                encoding="utf-8",
            )
            (math_dir / "DeterministicRNG.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            module_ids = list(manifests.keys())
            errors = check_include_boundaries(root, module_ids, manifests, INCLUDE_ENFORCED_MODULES)
            self.assertEqual(errors, [])

    def test_math_rejects_relative_escape_to_sibling(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_min_registry(root)
            math_dir = root / "core/math"
            log_dir = root / "core/logging"
            math_dir.mkdir(parents=True)
            log_dir.mkdir(parents=True)
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (math_dir / "Bad.cpp").write_text(
                '#include "../logging/Logger.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            module_ids = list(manifests.keys())
            errors = check_include_boundaries(root, module_ids, manifests, frozenset({"core.math"}))
            self.assertTrue(errors, "expected violation for ../logging include")

    def test_influence_rejects_undeclared_sibling_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_influence_registry(root)
            inf_dir = root / "core/influence"
            inf_dir.mkdir(parents=True)
            (inf_dir / "Bad.cpp").write_text('#include "core/logging/Logger.h"\n', encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence"})
            )
            self.assertTrue(any("core/logging" in e for e in errors), errors)

    def test_influence_allows_math_and_declared_interfaces(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_influence_registry(root)
            math_dir = root / "core/math"
            ifaces = root / "core/interfaces"
            inf_dir = root / "core/influence"
            math_dir.mkdir(parents=True)
            ifaces.mkdir(parents=True)
            inf_dir.mkdir(parents=True)
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (ifaces / "IGameUnit.h").write_text("#pragma once\n", encoding="utf-8")
            (inf_dir / "Ok.cpp").write_text(
                '#include "core/math/Vector2.h"\n'
                '#include "core/interfaces/IGameUnit.h"\n'
                '#include "InfluenceFieldItem.h"\n',
                encoding="utf-8",
            )
            (inf_dir / "InfluenceFieldItem.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence"})
            )
            self.assertEqual(errors, [])

    def test_influence_rejects_relative_escape(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_influence_registry(root)
            inf_dir = root / "core/influence"
            ai_dir = root / "core/ai"
            inf_dir.mkdir(parents=True)
            ai_dir.mkdir(parents=True)
            (ai_dir / "BehaviorFactory.h").write_text("#pragma once\n", encoding="utf-8")
            (inf_dir / "Bad.cpp").write_text('#include "../ai/BehaviorFactory.h"\n', encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence"})
            )
            self.assertTrue(errors, errors)

    def test_manifest_rejects_mutual_requires_cycle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_cyclic_registry(root)
            modules_dir = root / "docs/agentic/modules"
            registry = json.loads((modules_dir / "registry.json").read_text())
            manifests = self._manifests(root)
            errors = detect_requires_cycles(registry["modules"], manifests)
            self.assertTrue(any("cycle" in e for e in errors), errors)

    def test_manifest_rejects_three_node_requires_cycle(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_three_node_cycle_registry(root)
            registry = json.loads(
                (root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
            )
            errors = detect_requires_cycles(registry["modules"], self._manifests(root))
            self.assertEqual(len(errors), 1, errors)
            self.assertIn("cycle", errors[0])

    def test_option2_dag_fixture_passes_cycle_and_include_checks(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_option2_dag_registry(root)
            registry = json.loads(
                (root / "docs/agentic/modules/registry.json").read_text(encoding="utf-8")
            )
            manifests = self._manifests(root)
            module_ids = registry["modules"]
            self.assertEqual(detect_requires_cycles(module_ids, manifests), [])
            types_dir = root / "core/influence_types"
            (types_dir / "Ok.cpp").write_text('#include "core/math/Vector2.h"\n', encoding="utf-8")
            (root / "core/math/Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            errors = check_include_boundaries(
                root, module_ids, manifests, frozenset({"core.influence.types"})
            )
            self.assertEqual(errors, [])

    def test_influence_allows_relative_include_into_declared_types(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_influence_registry(root)
            types_dir = root / "core/influence_types"
            inf_dir = root / "core/influence"
            types_dir.mkdir(parents=True)
            inf_dir.mkdir(parents=True)
            (types_dir / "InfluenceFieldItem.h").write_text("#pragma once\n", encoding="utf-8")
            (inf_dir / "Ok.cpp").write_text(
                '#include "../influence_types/InfluenceFieldItem.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence"})
            )
            self.assertEqual(errors, [])

    def test_influence_rejects_undeclared_core_interfaces_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_influence_registry(root, include_interfaces_require=False)
            inf_dir = root / "core/influence"
            ifaces = root / "core/interfaces"
            inf_dir.mkdir(parents=True)
            ifaces.mkdir(parents=True)
            (ifaces / "IGameUnit.h").write_text("#pragma once\n", encoding="utf-8")
            (inf_dir / "Bad.cpp").write_text(
                '#include "core/interfaces/IGameUnit.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence"})
            )
            self.assertTrue(any("core/interfaces" in e for e in errors), errors)

    def test_influence_types_rejects_interfaces_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_types_registry(root)
            types_dir = root / "core/influence_types"
            ifaces = root / "core/interfaces"
            types_dir.mkdir(parents=True)
            ifaces.mkdir(parents=True)
            (ifaces / "IGameUnit.h").write_text("#pragma once\n", encoding="utf-8")
            (types_dir / "Bad.cpp").write_text(
                '#include "core/interfaces/IGameUnit.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.influence.types"})
            )
            self.assertTrue(any("core/interfaces" in e for e in errors), errors)

    def test_logging_rejects_sibling_core_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_logging_registry(root)
            log_dir = root / "core/logging"
            math_dir = root / "core/math"
            math_dir.mkdir(parents=True)
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (log_dir / "Bad.cpp").write_text(
                '#include "core/math/Vector2.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.logging"})
            )
            self.assertTrue(any("core/math" in e for e in errors), errors)

    def test_logging_allows_own_headers_and_stdlib(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_logging_registry(root)
            log_dir = root / "core/logging"
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (log_dir / "Ok.cpp").write_text(
                '#include "Logger.h"\n#include <iostream>\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.logging"})
            )
            self.assertEqual(errors, [])

    def test_interfaces_rejects_sibling_core_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_interfaces_registry(root)
            ifaces_dir = root / "core/interfaces"
            log_dir = root / "core/logging"
            log_dir.mkdir(parents=True)
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (ifaces_dir / "Bad.cpp").write_text(
                '#include "core/logging/Logger.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.interfaces"})
            )
            self.assertTrue(any("core/logging" in e for e in errors), errors)

    def test_interfaces_allows_math_and_types_and_own_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_interfaces_registry(root)
            math_dir = root / "core/math"
            types_dir = root / "core/influence_types"
            ifaces_dir = root / "core/interfaces"
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (types_dir / "InfluenceFieldItem.h").write_text("#pragma once\n", encoding="utf-8")
            (ifaces_dir / "Ok.cpp").write_text(
                '#include "IGameUnit.h"\n'
                '#include "core/math/Vector2.h"\n'
                '#include "core/influence_types/InfluenceFieldItem.h"\n'
                '#include <vector>\n',
                encoding="utf-8",
            )
            (ifaces_dir / "IGameUnit.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.interfaces"})
            )
            self.assertEqual(errors, [])

    def test_gameloop_rejects_undeclared_sibling_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_gameloop_registry(root)
            gl_dir = root / "core/gameloop"
            log_dir = root / "core/logging"
            log_dir.mkdir(parents=True)
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (gl_dir / "Bad.cpp").write_text(
                '#include "core/logging/Logger.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.gameloop"})
            )
            self.assertTrue(any("core/logging" in e for e in errors), errors)

    def test_gameloop_allows_declared_math_and_own_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_gameloop_registry(root)
            math_dir = root / "core/math"
            gl_dir = root / "core/gameloop"
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (gl_dir / "Ok.cpp").write_text(
                '#include "GameLogicEngine.h"\n'
                '#include "core/math/Vector2.h"\n'
                '#include <algorithm>\n',
                encoding="utf-8",
            )
            (gl_dir / "GameLogicEngine.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.gameloop"})
            )
            self.assertEqual(errors, [])

    def test_map_rejects_undeclared_sibling_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_map_registry(root)
            map_dir = root / "core/map"
            log_dir = root / "core/logging"
            log_dir.mkdir(parents=True)
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (map_dir / "Bad.cpp").write_text(
                '#include "core/logging/Logger.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.map"})
            )
            self.assertTrue(any("core/logging" in e for e in errors), errors)

    def test_map_allows_declared_deps_and_own_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_map_registry(root)
            math_dir = root / "core/math"
            types_dir = root / "core/influence_types"
            map_dir = root / "core/map"
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (types_dir / "InfluenceFieldItem.h").write_text("#pragma once\n", encoding="utf-8")
            (map_dir / "Ok.cpp").write_text(
                '#include "Map.h"\n'
                '#include "core/math/Vector2.h"\n'
                '#include "core/influence_types/InfluenceFieldItem.h"\n'
                '#include <vector>\n',
                encoding="utf-8",
            )
            (map_dir / "Map.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.map"})
            )
            self.assertEqual(errors, [])

    def test_ai_rejects_undeclared_sibling_include(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_ai_registry(root)
            ai_dir = root / "core/ai"
            gs_dir = root / "core/gamestate"
            gs_dir.mkdir(parents=True)
            (gs_dir / "RTSGameState.h").write_text("#pragma once\n", encoding="utf-8")
            (ai_dir / "Bad.cpp").write_text(
                '#include "core/gamestate/RTSGameState.h"\n',
                encoding="utf-8",
            )
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.ai"})
            )
            self.assertTrue(any("core/gamestate" in e for e in errors), errors)

    def test_ai_allows_declared_deps_and_own_headers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_ai_registry(root)
            math_dir = root / "core/math"
            types_dir = root / "core/influence_types"
            log_dir = root / "core/logging"
            ifaces = root / "core/interfaces"
            ai_dir = root / "core/ai"
            (math_dir / "Vector2.h").write_text("#pragma once\n", encoding="utf-8")
            (types_dir / "InfluenceFieldItem.h").write_text("#pragma once\n", encoding="utf-8")
            (log_dir / "Logger.h").write_text("#pragma once\n", encoding="utf-8")
            (ifaces / "IGameUnit.h").write_text("#pragma once\n", encoding="utf-8")
            (ifaces / "IBehavior.h").write_text("#pragma once\n", encoding="utf-8")
            (ai_dir / "Ok.cpp").write_text(
                '#include "BehaviorContext.h"\n'
                '#include "core/math/Vector2.h"\n'
                '#include "core/influence_types/InfluenceFieldItem.h"\n'
                '#include "core/logging/Logger.h"\n'
                '#include "core/interfaces/IGameUnit.h"\n'
                '#include <vector>\n',
                encoding="utf-8",
            )
            (ai_dir / "BehaviorContext.h").write_text("#pragma once\n", encoding="utf-8")
            manifests = self._manifests(root)
            errors = check_include_boundaries(
                root, list(manifests.keys()), manifests, frozenset({"core.ai"})
            )
            self.assertEqual(errors, [])

    @staticmethod
    def _write_interfaces_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.influence.types", "core.interfaces"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.types.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence.types",
                    "version": "1.0.0",
                    "path": "core/influence_types",
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IGameUnit"],
                    "requires": {
                        "core.math": "^1.0.0",
                        "core.influence.types": "^1.0.0",
                    },
                }
            ),
            encoding="utf-8",
        )
        (root / "core/math").mkdir(parents=True, exist_ok=True)
        (root / "core/influence_types").mkdir(parents=True, exist_ok=True)
        (root / "core/interfaces").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_gameloop_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.gameloop"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.gameloop.module.json").write_text(
            json.dumps(
                {
                    "id": "core.gameloop",
                    "version": "1.0.0",
                    "path": "core/gameloop",
                    "exports": ["GameLogicEngine"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (root / "core/math").mkdir(parents=True, exist_ok=True)
        (root / "core/gameloop").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_map_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.influence.types", "core.interfaces", "core.map"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.types.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence.types",
                    "version": "1.0.0",
                    "path": "core/influence_types",
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IGameUnit"],
                    "requires": {"core.math": "^1.0.0", "core.influence.types": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.map.module.json").write_text(
            json.dumps(
                {
                    "id": "core.map",
                    "version": "1.0.0",
                    "path": "core/map",
                    "exports": ["Map"],
                    "requires": {
                        "core.math": "^1.0.0",
                        "core.interfaces": "^1.0.0",
                        "core.influence.types": "^1.0.0",
                    },
                }
            ),
            encoding="utf-8",
        )
        (root / "core/math").mkdir(parents=True, exist_ok=True)
        (root / "core/influence_types").mkdir(parents=True, exist_ok=True)
        (root / "core/interfaces").mkdir(parents=True, exist_ok=True)
        (root / "core/map").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_ai_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {
            "modules": [
                "core.math",
                "core.influence.types",
                "core.interfaces",
                "core.logging",
                "core.ai",
            ]
        }
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.types.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence.types",
                    "version": "1.0.0",
                    "path": "core/influence_types",
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IGameUnit"],
                    "requires": {"core.math": "^1.0.0", "core.influence.types": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.logging.module.json").write_text(
            json.dumps(
                {
                    "id": "core.logging",
                    "version": "1.0.0",
                    "path": "core/logging",
                    "exports": ["Logger"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.ai.module.json").write_text(
            json.dumps(
                {
                    "id": "core.ai",
                    "version": "1.0.0",
                    "path": "core/ai",
                    "exports": ["StateMachine"],
                    "requires": {
                        "core.math": "^1.0.0",
                        "core.interfaces": "^1.0.0",
                        "core.logging": "^1.0.0",
                        "core.influence.types": "^1.0.0",
                    },
                }
            ),
            encoding="utf-8",
        )
        (root / "core/math").mkdir(parents=True, exist_ok=True)
        (root / "core/influence_types").mkdir(parents=True, exist_ok=True)
        (root / "core/interfaces").mkdir(parents=True, exist_ok=True)
        (root / "core/logging").mkdir(parents=True, exist_ok=True)
        (root / "core/ai").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_logging_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.logging"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.logging.module.json").write_text(
            json.dumps(
                {
                    "id": "core.logging",
                    "version": "1.0.0",
                    "path": "core/logging",
                    "exports": ["Logger"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (root / "core/logging").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_three_node_cycle_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.a", "core.b", "core.c"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        for mid, req in (
            ("core.a", {"core.c": "^1.0.0"}),
            ("core.b", {"core.a": "^1.0.0"}),
            ("core.c", {"core.b": "^1.0.0"}),
        ):
            (modules_dir / f"{mid}.module.json").write_text(
                json.dumps(
                    {
                        "id": mid,
                        "version": "1.0.0",
                        "path": f"core/{mid.replace('.', '/')}",
                        "exports": ["X"],
                        "requires": req,
                    }
                ),
                encoding="utf-8",
            )
            (root / f"core/{mid.replace('.', '/')}").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_option2_dag_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {
            "modules": [
                "core.math",
                "core.influence.types",
                "core.interfaces",
                "core.influence",
            ]
        }
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (root / "core/math").mkdir(parents=True, exist_ok=True)
        (root / "core/influence_types").mkdir(parents=True, exist_ok=True)
        (root / "core/interfaces").mkdir(parents=True, exist_ok=True)
        (root / "core/influence").mkdir(parents=True, exist_ok=True)
        specs = {
            "core.math": ("core/math", [], {}),
            "core.influence.types": (
                "core/influence_types",
                ["InfluenceFieldItem"],
                {"core.math": "^1.0.0"},
            ),
            "core.interfaces": (
                "core/interfaces",
                ["IBehavior"],
                {"core.math": "^1.0.0", "core.influence.types": "^1.0.0"},
            ),
            "core.influence": (
                "core/influence",
                ["InfluenceFieldCalculator"],
                {
                    "core.math": "^1.0.0",
                    "core.influence.types": "^1.0.0",
                    "core.interfaces": "^1.0.0",
                },
            ),
        }
        for mid, (path, exports, requires) in specs.items():
            (modules_dir / f"{mid}.module.json").write_text(
                json.dumps(
                    {
                        "id": mid,
                        "version": "1.0.0",
                        "path": path,
                        "exports": exports,
                        "requires": requires,
                    }
                ),
                encoding="utf-8",
            )

    @staticmethod
    def _write_cyclic_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.interfaces", "core.influence"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IBehavior"],
                    "requires": {"core.influence": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence",
                    "version": "1.0.0",
                    "path": "core/influence",
                    "exports": ["TerritoryCalculator"],
                    "requires": {"core.interfaces": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )

    @staticmethod
    def _write_types_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.influence.types"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.types.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence.types",
                    "version": "1.0.0",
                    "path": "core/influence_types",
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )

    @staticmethod
    def _write_influence_registry(root: Path, *, include_interfaces_require: bool = True) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {
            "modules": [
                "core.math",
                "core.influence.types",
                "core.interfaces",
                "core.influence",
            ]
        }
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.influence.types.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence.types",
                    "version": "1.0.0",
                    "path": "core/influence_types",
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IGameUnit"],
                    "requires": {"core.math": "^1.0.0", "core.influence.types": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )
        inf_requires = {
            "core.math": "^1.0.0",
            "core.influence.types": "^1.0.0",
        }
        if include_interfaces_require:
            inf_requires["core.interfaces"] = "^1.0.0"
        (modules_dir / "core.influence.module.json").write_text(
            json.dumps(
                {
                    "id": "core.influence",
                    "version": "1.0.0",
                    "path": "core/influence",
                    "exports": ["InfluenceFieldCalculator"],
                    "requires": inf_requires,
                }
            ),
            encoding="utf-8",
        )

    @staticmethod
    def _write_two_module_registry(root: Path, *, extra_require: bool = False) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.a", "core.b"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.a.module.json").write_text(
            json.dumps(
                {
                    "id": "core.a",
                    "version": "1.0.0",
                    "path": "core/a",
                    "exports": ["A"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        b_requires = {"core.a": "^1.0.0"} if extra_require else {}
        (modules_dir / "core.b.module.json").write_text(
            json.dumps(
                {
                    "id": "core.b",
                    "version": "1.0.0",
                    "path": "core/b",
                    "exports": ["B"],
                    "requires": b_requires,
                }
            ),
            encoding="utf-8",
        )
        (root / "core/a").mkdir(parents=True, exist_ok=True)
        (root / "core/b").mkdir(parents=True, exist_ok=True)

    @staticmethod
    def _write_min_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.logging"]}
        (modules_dir / "registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (modules_dir / "core.math.module.json").write_text(
            json.dumps(
                {
                    "id": "core.math",
                    "version": "1.0.0",
                    "path": "core/math",
                    "exports": ["Vector2"],
                    "requires": {},
                }
            ),
            encoding="utf-8",
        )
        (modules_dir / "core.logging.module.json").write_text(
            json.dumps(
                {
                    "id": "core.logging",
                    "version": "1.0.0",
                    "path": "core/logging",
                    "exports": ["Logger"],
                    "requires": {"core.math": "^1.0.0"},
                }
            ),
            encoding="utf-8",
        )

    @staticmethod
    def _manifests(root: Path) -> dict:
        modules_dir = root / "docs/agentic/modules"
        out = {}
        for path in modules_dir.glob("*.module.json"):
            data = json.loads(path.read_text(encoding="utf-8"))
            out[data["id"]] = data
        return out


if __name__ == "__main__":
    unittest.main()
