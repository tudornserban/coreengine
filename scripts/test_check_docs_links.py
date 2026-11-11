#!/usr/bin/env python3
"""Regression tests for scripts/check_docs_links.py."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_docs_links import check_file, run_checks


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


if __name__ == "__main__":
    unittest.main()
