#!/usr/bin/env python3
"""Regression tests for BUILD_SDL2 / BUILD_RAYLIB configure-time fatal errors."""
from __future__ import annotations

import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_platform_stub_options import (  # noqa: E402
    RAYLIB_FATAL,
    SDL2_FATAL,
    STUB_SNIPPET,
    configure_default_succeeds,
    configure_should_fail,
    run_checks,
)


class PlatformStubOptionsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[1]
        text = (cls.repo_root / "CMakeLists.txt").read_text(encoding="utf-8")
        sdl_block = re.search(
            r"if\(BUILD_SDL2\)(.*?)endif\(\)",
            text,
            re.DOTALL,
        )
        assert sdl_block is not None, "BUILD_SDL2 block not found in CMakeLists.txt"
        cls.sdl_block = sdl_block.group(1)
        ray_block = re.search(
            r"if\(BUILD_RAYLIB\)(.*?)endif\(\)",
            text,
            re.DOTALL,
        )
        assert ray_block is not None, "BUILD_RAYLIB block not found in CMakeLists.txt"
        assert ray_block is not None
        cls.ray_block = ray_block.group(1)

    def test_build_sdl2_configure_fails_with_message(self) -> None:
        ok, detail = configure_should_fail(self.repo_root, "BUILD_SDL2", SDL2_FATAL)
        self.assertTrue(ok, detail)
        self.assertIn("COREENGINE_BUILD_SDL2_VIEWER", detail)

    def test_build_raylib_configure_fails_with_message(self) -> None:
        ok, detail = configure_should_fail(self.repo_root, "BUILD_RAYLIB", RAYLIB_FATAL)
        self.assertTrue(ok, detail)

    def test_default_configure_succeeds(self) -> None:
        ok, detail = configure_default_succeeds(self.repo_root)
        self.assertTrue(ok, detail)

    def test_repo_gate_passes(self) -> None:
        ok, errors = run_checks(self.repo_root)
        self.assertTrue(ok, errors)

    def test_fatal_error_before_find_package_sdl2(self) -> None:
        self.assertIn("message(FATAL_ERROR", self.sdl_block)
        self.assertNotIn("find_package(SDL2", self.sdl_block)

    def test_fatal_error_before_find_package_raylib(self) -> None:
        self.assertIn("message(FATAL_ERROR", self.ray_block)
        self.assertNotIn("find_package(raylib", self.ray_block)

    def test_message_contains_header_only_stub(self) -> None:
        self.assertIn(STUB_SNIPPET, self.sdl_block)
        self.assertIn(STUB_SNIPPET, self.ray_block)


class PlatformStubOptionsMutationTest(unittest.TestCase):
    """Mutations against a temp copy of CMakeLists.txt (not the live tree)."""

    def _mutate_and_configure_block(
        self,
        start_marker: str,
        end_marker: str,
        replacement: str,
        flag: str,
    ) -> subprocess.CompletedProcess[str]:
        repo_root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "repo"
            subprocess.run(
                ["git", "clone", "--local", str(repo_root), str(root)],
                check=True,
                capture_output=True,
            )
            cmake = root / "CMakeLists.txt"
            text = cmake.read_text(encoding="utf-8")
            start = text.index(start_marker)
            end = text.index(end_marker, start) + len(end_marker)
            cmake.write_text(text[:start] + replacement + text[end:], encoding="utf-8")
            build = root / "build"
            build.mkdir()
            import os
            import shutil as _shutil

            cxx = os.environ.get("CXX", "g++")
            cmd = [
                "cmake",
                "-S",
                str(root),
                "-B",
                str(build),
                f"-DCMAKE_CXX_COMPILER={cxx}",
                f"-D{flag}=ON",
            ]
            if _shutil.which("ninja") is not None:
                cmd.extend(["-G", "Ninja"])
            return subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=False,
            )

    def _mutate_and_configure(
        self,
        old: str,
        new: str,
        flag: str,
    ) -> subprocess.CompletedProcess[str]:
        repo_root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "repo"
            subprocess.run(
                ["git", "clone", "--local", str(repo_root), str(root)],
                check=True,
                capture_output=True,
            )
            cmake = root / "CMakeLists.txt"
            text = cmake.read_text(encoding="utf-8")
            cmake.write_text(text.replace(old, new, 1), encoding="utf-8")
            build = root / "build"
            build.mkdir()
            import os
            import shutil as _shutil

            cxx = os.environ.get("CXX", "g++")
            cmd = [
                "cmake",
                "-S",
                str(root),
                "-B",
                str(build),
                f"-DCMAKE_CXX_COMPILER={cxx}",
                f"-D{flag}=ON",
            ]
            if _shutil.which("ninja") is not None:
                cmd.extend(["-G", "Ninja"])
            return subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=False,
            )

    def test_mutation_removed_fatal_error_sdl2_fails_differently(self) -> None:
        legacy_sdl2 = """    if(BUILD_SDL2)
        find_package(SDL2 REQUIRED)
        file(GLOB SDL2_SOURCES "platform/sdl2/*.cpp")
        add_library(coreengine_sdl2 STATIC ${SDL2_SOURCES})
        target_link_libraries(coreengine_sdl2 PUBLIC coreengine SDL2::SDL2)
        target_include_directories(coreengine_sdl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    endif()"""
        fatal_sdl2_start = '    if(BUILD_SDL2)\n        message(FATAL_ERROR'
        proc = self._mutate_and_configure_block(
            fatal_sdl2_start,
            "    endif()",
            legacy_sdl2,
            "BUILD_SDL2",
        )
        combined = proc.stdout + proc.stderr
        norm = re.sub(r"\\s+", " ", combined)
        self.assertNotEqual(proc.returncode, 0)
        self.assertNotIn("header-only stub", norm)
        self.assertTrue(
            "No SOURCES" in combined or "SDL2" in combined,
            "expected legacy configure failure without stub message",
        )

    def test_mutation_message_text_sdl2_caught_by_configure_check(self) -> None:
        proc = self._mutate_and_configure(
            SDL2_FATAL,
            SDL2_FATAL.replace(STUB_SNIPPET, "placeholder stub"),
            "BUILD_SDL2",
        )
        combined = proc.stdout + proc.stderr
        if proc.returncode == 0:
            self.fail("expected configure failure")
        self.assertNotIn(SDL2_FATAL, combined)


if __name__ == "__main__":
    unittest.main()
