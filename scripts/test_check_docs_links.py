#!/usr/bin/env python3
"""Regression tests for scripts/check_docs_links.py and check-module-deps.sh wiring."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_docs_links import (
    check_file,
    iter_doc_files,
    resolve_link,
    run_checks,
)


class CheckDocsLinksTest(unittest.TestCase):
    def test_real_repo_passes(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        ok, errors = run_checks(repo_root)
        self.assertTrue(ok, errors)

    def test_broken_relative_link_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "README.md").write_text("[x](docs/missing.md)\n", encoding="utf-8")
            (root / "docs").mkdir()
            ok, errors = run_checks(root)
            self.assertFalse(ok)
            self.assertTrue(any("missing.md" in e for e in errors))

    def test_parent_relative_link_resolves(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            docs = root / "docs"
            docs.mkdir()
            target = docs / "ok.md"
            target.write_text("# OK\n", encoding="utf-8")
            readme = root / "README.md"
            readme.write_text("[ok](docs/ok.md)\n", encoding="utf-8")
            errors = check_file(readme, root)
            self.assertEqual(errors, [])

    def test_scan_includes_docs_and_module_md_not_readme_only(self) -> None:
        """If only README is valid, a broken link under docs/ must still fail."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "README.md").write_text("# root\n", encoding="utf-8")
            agentic = root / "docs" / "agentic"
            agentic.mkdir(parents=True)
            (agentic / "broken.md").write_text("[bad](../nope/MODULE.md)\n", encoding="utf-8")
            mod = root / "core" / "math"
            mod.mkdir(parents=True)
            (mod / "MODULE.md").write_text("# math\n", encoding="utf-8")
            ok, errors = run_checks(root)
            self.assertFalse(ok, "expected failure from docs/agentic/broken.md")
            self.assertTrue(any("docs/agentic" in e for e in errors))
            self.assertTrue(any("nope" in e or "MODULE.md" in e for e in errors))

    def test_iter_doc_files_visits_docs_and_module_globs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "README.md").write_text("x", encoding="utf-8")
            (root / "docs").mkdir()
            (root / "docs" / "x.md").write_text("x", encoding="utf-8")
            (root / "core" / "math").mkdir(parents=True)
            (root / "core" / "math" / "MODULE.md").write_text("x", encoding="utf-8")
            paths = {p.relative_to(root).as_posix() for p in iter_doc_files(root)}
            self.assertIn("docs/x.md", paths)
            self.assertIn("core/math/MODULE.md", paths)
            self.assertGreaterEqual(len(paths), 3)

    def test_broken_parent_relative_link_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            agentic = root / "docs" / "agentic"
            agentic.mkdir(parents=True)
            (agentic / "child.md").write_text("[up](../missing.md)\n", encoding="utf-8")
            (root / "README.md").write_text("# ok\n", encoding="utf-8")
            ok, errors = run_checks(root)
            self.assertFalse(ok)
            self.assertTrue(any("../missing.md" in e for e in errors))

    def test_missing_file_with_anchor_still_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "README.md").write_text(
                "[sec](docs/missing.md#section)\n", encoding="utf-8"
            )
            (root / "docs").mkdir()
            ok, errors = run_checks(root)
            self.assertFalse(ok)
            self.assertTrue(any("missing.md" in e for e in errors))

    def test_resolve_link_splits_anchor_but_checks_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "README.md"
            resolved, frag = resolve_link(src, "docs/a.md#foo", root)
            self.assertIsNotNone(resolved)
            self.assertEqual(frag, "foo")
            self.assertFalse(resolved.exists())

    def test_check_module_deps_sh_invokes_docs_link_checker(self) -> None:
        repo_root = Path(__file__).resolve().parents[1]
        sh = repo_root / "scripts" / "check-module-deps.sh"
        text = sh.read_text(encoding="utf-8")
        self.assertIn("check_docs_links.py", text)
        self.assertIn("test_check_docs_links.py", text)

if __name__ == "__main__":
    unittest.main()
