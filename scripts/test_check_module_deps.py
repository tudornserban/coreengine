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
    run_checks,
)


class CheckModuleDepsRegression(unittest.TestCase):
    def test_real_repo_passes(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        result = run_checks(repo_root)
        self.assertTrue(result.ok, result.errors)

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

    @staticmethod
    def _write_influence_registry(root: Path) -> None:
        modules_dir = root / "docs/agentic/modules"
        modules_dir.mkdir(parents=True)
        registry = {"modules": ["core.math", "core.interfaces", "core.influence"]}
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
        (modules_dir / "core.interfaces.module.json").write_text(
            json.dumps(
                {
                    "id": "core.interfaces",
                    "version": "1.0.0",
                    "path": "core/interfaces",
                    "exports": ["IGameUnit"],
                    "requires": {"core.math": "^1.0.0", "core.influence": "^1.0.0"},
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
                    "exports": ["InfluenceFieldItem"],
                    "requires": {"core.math": "^1.0.0", "core.interfaces": "^1.0.0"},
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
