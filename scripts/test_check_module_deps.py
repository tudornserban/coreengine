#!/usr/bin/env python3
"""Regression tests for scripts/check_module_deps.py (manifest + core.math includes)."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_module_deps import (
    INCLUDE_ENFORCED_MODULES,
    check_include_boundaries,
    detect_requires_cycles,
    run_checks,
)


class CheckModuleDepsRegression(unittest.TestCase):
    def test_real_repo_passes(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        result = run_checks(repo_root)
        self.assertTrue(result.ok, result.errors)

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
