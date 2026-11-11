#!/usr/bin/env python3
"""Verify BUILD_SDL2 / BUILD_RAYLIB fail at configure with explicit stub messages."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Iterable

STUB_SNIPPET = "header-only stub"

SDL2_FATAL = (
    "BUILD_SDL2 is not buildable: platform/sdl2 is a header-only stub "
    "with no .cpp files. Leave BUILD_SDL2=OFF. "
    "For a windowed SDL2 loop use -DCOREENGINE_BUILD_SDL2_VIEWER=ON "
    "(docs/VIEWER.md)."
)

RAYLIB_FATAL = (
    "BUILD_RAYLIB is not buildable: platform/raylib is a header-only stub "
    "with no .cpp files. Leave BUILD_RAYLIB=OFF."
)


def _cmake_configure(
    repo_root: Path,
    build_dir: Path,
    extra_args: Iterable[str],
) -> subprocess.CompletedProcess[str]:
    build_dir.mkdir(parents=True, exist_ok=True)
    cxx = os.environ.get("CXX", "g++")
    cmd = [
        "cmake",
        "-S",
        str(repo_root),
        "-B",
        str(build_dir),
        f"-DCMAKE_CXX_COMPILER={cxx}",
    ]
    if shutil.which("ninja") is not None:
        cmd.extend(["-G", "Ninja"])
    cmd.extend(extra_args)
    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
    )


def _normalize_cmake_message(text: str) -> str:
    """Collapse CMake line wraps so multi-line FATAL_ERROR strings compare reliably."""
    return re.sub(r"\s+", " ", text).strip()


def configure_should_fail(
    repo_root: Path,
    flag: str,
    expected_substring: str,
) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory(prefix="ce-stub-check-") as tmp:
        build_dir = Path(tmp) / "build"
        result = _cmake_configure(repo_root, build_dir, [f"-D{flag}=ON"])
        combined = (result.stdout or "") + (result.stderr or "")
        norm = _normalize_cmake_message(combined)
        if result.returncode == 0:
            return False, f"{flag}=ON succeeded but should have failed configure"
        expected_norm = _normalize_cmake_message(expected_substring)
        if expected_norm not in norm:
            return (
                False,
                f"{flag}=ON failed as expected but output missing {expected_substring!r}:\n"
                f"{combined}",
            )
        if STUB_SNIPPET not in norm:
            return False, f"{flag}=ON output missing {STUB_SNIPPET!r}:\n{combined}"
        if f"{flag} is not buildable" not in norm:
            return False, f"{flag}=ON output missing 'is not buildable':\n{combined}"
        return True, combined


def configure_default_succeeds(repo_root: Path) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory(prefix="ce-default-cfg-") as tmp:
        build_dir = Path(tmp) / "build"
        result = _cmake_configure(repo_root, build_dir, [])
        combined = (result.stdout or "") + (result.stderr or "")
        if result.returncode != 0:
            return False, f"default configure failed:\n{combined}"
        return True, combined


def viewer_configure_succeeds_or_skip(repo_root: Path) -> tuple[str, str]:
    """Returns ('ok'|'skip'|'fail', detail). Skip when SDL2 is not installed."""
    if shutil.which("sdl2-config") is None:
        if shutil.which("pkg-config") is None:
            return "skip", "SDL2 not installed; skipping viewer configure check"
        pkg = subprocess.run(
            ["pkg-config", "--exists", "sdl2"],
            capture_output=True,
            check=False,
        )
        if pkg.returncode != 0:
            return "skip", "SDL2 not installed; skipping viewer configure check"
    with tempfile.TemporaryDirectory(prefix="ce-viewer-cfg-") as tmp:
        build_dir = Path(tmp) / "build"
        result = _cmake_configure(
            repo_root,
            build_dir,
            ["-DCOREENGINE_BUILD_SDL2_VIEWER=ON"],
        )
        combined = (result.stdout or "") + (result.stderr or "")
        if result.returncode != 0:
            return "fail", f"COREENGINE_BUILD_SDL2_VIEWER=ON configure failed:\n{combined}"
        return "ok", combined


def run_checks(repo_root: Path) -> tuple[bool, list[str]]:
    errors: list[str] = []
    for flag, msg in (
        ("BUILD_SDL2", SDL2_FATAL),
        ("BUILD_RAYLIB", RAYLIB_FATAL),
    ):
        ok, detail = configure_should_fail(repo_root, flag, msg)
        if not ok:
            errors.append(detail)

    ok, detail = configure_default_succeeds(repo_root)
    if not ok:
        errors.append(detail)

    status, detail = viewer_configure_succeeds_or_skip(repo_root)
    if status == "fail":
        errors.append(detail)

    return len(errors) == 0, errors


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    ok, errors = run_checks(repo_root)
    if not ok:
        for err in errors:
            print(err, flush=True)
        return 1
    print("[check_platform_stub_options] OK", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
