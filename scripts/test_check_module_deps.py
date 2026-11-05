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
            errors = check_include_boundaries(root, module_ids, manifests, INCLUDE_ENFORCED_MODULES)
            self.assertTrue(errors, "expected violation for ../logging include")

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
