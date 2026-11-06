#!/usr/bin/env python3
"""Regression tests for scripts/build_dir_guard.py (no repo/home deletion)."""
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from build_dir_guard import resolve_build_dir, safe_rm_build_dir


class BuildDirGuardTest(unittest.TestCase):
    def test_allows_repo_local_build_math(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            target = resolve_build_dir(repo, repo / "build-math")
            self.assertEqual(target, (repo / "build-math").resolve())

    def test_rejects_repo_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, repo)

    def test_rejects_outside_repo(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            outside = Path(tmp) / ".." / "outside-build-math"
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, outside.resolve())

    def test_rejects_nested_build_dir(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            nested = repo / "build-math" / "nested"
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, nested)

    def test_rejects_non_build_prefix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, repo / "output")

    def test_rejects_empty_relative(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, "")

    def test_safe_rm_removes_only_valid_build_tree(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            sentinel = repo / "SOURCE_SENTINEL.txt"
            sentinel.write_text("keep", encoding="utf-8")
            build = repo / "build-logging"
            build.mkdir()
            (build / "CMakeCache.txt").write_text("cache", encoding="utf-8")
            removed = safe_rm_build_dir(repo, "build-logging")
            self.assertEqual(removed, build.resolve())
            self.assertFalse(build.exists())
            self.assertTrue(sentinel.is_file())
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "keep")

    def test_safe_rm_rejects_dangerous_override_without_deleting_sentinel(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            sentinel = repo / "SOURCE_SENTINEL.txt"
            sentinel.write_text("keep", encoding="utf-8")
            with self.assertRaises(ValueError):
                safe_rm_build_dir(repo, repo)  # type: ignore[arg-type]
            self.assertTrue(sentinel.is_file())
            with self.assertRaises(ValueError):
                safe_rm_build_dir(repo, repo.parent / "build-evil")
            self.assertTrue(sentinel.is_file())

    def test_rejects_symlink_escape(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            outside = Path(tmp) / "outside"
            outside.mkdir()
            link = repo / "build-math"
            link.symlink_to(outside, target_is_directory=True)
            with self.assertRaises(ValueError):
                resolve_build_dir(repo, link)


if __name__ == "__main__":
    unittest.main()