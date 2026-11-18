#!/usr/bin/env python3
"""Host platform detection: cmake probes, header macros, and toolchain failures."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PROBE = REPO / "scripts" / "probe_host_platform.cmake"

ANDROID_NDK_MISSING = (
    "Android builds need ANDROID_NDK or ANDROID_NDK_ROOT "
    "(CMake variable or environment) pointing at Android NDK r25 or newer."
)
MACOS_OSXCROSS_MISSING = (
    "macOS cross builds need OSXCROSS_ROOT "
    "(or the OSXCROSS_ROOT environment variable) "
    "pointing at an osxcross installation that contains bin/o64-clang++."
)
WINDOWS_MINGW_MISSING = (
    "Windows cross builds need MinGW-w64 (x86_64-w64-mingw32-g++ on PATH) or MINGW_ROOT."
)
ANDROID_TESTS_REJECTED = (
    "BUILD_TESTS is not supported when the target platform is android."
)


def _norm(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def _host_system() -> str:
    uname = getattr(os, "uname", None)
    if uname is None:
        return os.name
    return uname().sysname
    uname = getattr(os, "uname", None)
    if uname is None:
        return os.name
    return uname().sysname


def _run(cmd: list[str], env: dict[str, str] | None = None, stdin: str | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        input=stdin,
        capture_output=True,
        text=True,
        check=False,
        env=env,
        cwd=REPO,
    )


def _probe(system_name: str, expect: str, android: str = "OFF", emscripten: str = "OFF") -> subprocess.CompletedProcess[str]:
    return _run(
        [
            "cmake",
            f"-DPROBE_SYSTEM_NAME={system_name}",
            f"-DPROBE_EXPECT={expect}",
            f"-DPROBE_ANDROID={android}",
            f"-DPROBE_EMSCRIPTEN={emscripten}",
            "-P",
            str(PROBE),
        ]
    )


def _cleared_env() -> dict[str, str]:
    env = os.environ.copy()
    for key in (
        "ANDROID_NDK",
        "ANDROID_NDK_ROOT",
        "ANDROID_NDK_HOME",
        "OSXCROSS_ROOT",
        "MINGW_ROOT",
        "COREENGINE_MACOS_ARCH",
        "CC",
        "CXX",
    ):
        env.pop(key, None)
    return env


def _configure(build_dir: Path, extra: list[str], env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    cxx = os.environ.get("CXX", "g++")
    cmd = [
        "cmake",
        "-S",
        str(REPO),
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE=Release",
    ]
    # A toolchain file chooses the compiler. Passing the host g++ would undo that.
    if not any(arg.startswith("-DCMAKE_TOOLCHAIN_FILE=") for arg in extra):
        cmd.append(f"-DCMAKE_CXX_COMPILER={cxx}")
    if shutil.which("ninja") is not None:
        cmd.extend(["-G", "Ninja"])
    cmd.extend(extra)
    return _run(cmd, env=env)


class PlatformProbeTest(unittest.TestCase):
    def test_named_profiles(self) -> None:
        cases = [
            ("Linux", "linux", "OFF", "OFF"),
            ("Darwin", "macos", "OFF", "OFF"),
            ("Windows", "windows", "OFF", "OFF"),
            ("Android", "android", "OFF", "OFF"),
            ("Linux", "android", "TRUE", "OFF"),
            ("Emscripten", "emscripten", "OFF", "OFF"),
            ("Linux", "emscripten", "OFF", "TRUE"),
            ("FreeBSD", "generic", "OFF", "OFF"),
        ]
        for system_name, expect, android, emscripten in cases:
            with self.subTest(system_name=system_name, expect=expect, android=android):
                proc = _probe(system_name, expect, android, emscripten)
                self.assertEqual(
                    proc.returncode,
                    0,
                    proc.stdout + proc.stderr,
                )

    def test_header_macro_precedence(self) -> None:
        cxx = os.environ.get("CXX", "g++")
        if shutil.which(cxx) is None:
            self.skipTest(f"{cxx} is not installed")
        cases = {
            "linux": [],
            "windows": ["-D_WIN32"],
            "android": ["-D__ANDROID__"],
            "macos": ["-D__APPLE__", "-D__MACH__"],
            "emscripten": ["-D__EMSCRIPTEN__"],
        }
        for name, defines in cases.items():
            source = (
                '#include "platform/host/HostPlatform.h"\n'
                f'#define CE_EXPECT "{name}"\n'
                "static constexpr bool same(const char* a, const char* b) {\n"
                "  while (*a && *b) { if (*a != *b) return false; ++a; ++b; }\n"
                "  return *a == *b;\n"
                "}\n"
                "static_assert(same(COREENGINE_OS_NAME, CE_EXPECT), COREENGINE_OS_NAME);\n"
            )
            with self.subTest(name=name):
                proc = _run(
                    [cxx, "-std=c++17", "-fsyntax-only", "-I", str(REPO), "-x", "c++", "-"]
                    + defines,
                    stdin=source,
                )
                self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)


class PlatformConfigureTest(unittest.TestCase):
    def test_native_linux_cache_and_host_check_target(self) -> None:
        if _host_system() != "Linux":
            self.skipTest("native linux configure runs on Linux")
        with tempfile.TemporaryDirectory(prefix="ce-host-linux-") as tmp:
            build = Path(tmp) / "build"
            proc = _configure(build, ["-DBUILD_TESTS=OFF", "-DBUILD_FFI=OFF"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
            self.assertIn("COREENGINE_TARGET_PLATFORM:STRING=linux", cache)
            build_files = []
            for name in ("build.ninja", "Makefile"):
                path = build / name
                if path.exists():
                    build_files.append(path.read_text(encoding="utf-8", errors="replace"))
            self.assertTrue(build_files, "expected Ninja or Makefile after configure")
            self.assertTrue(
                any("coreengine_host_check" in text for text in build_files),
                "coreengine_host_check was not generated",
            )

    def test_android_toolchain_requires_ndk(self) -> None:
        with tempfile.TemporaryDirectory(prefix="ce-host-android-") as tmp:
            proc = _configure(
                Path(tmp) / "build",
                ["-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/android.cmake", "-DBUILD_TESTS=OFF"],
                env=_cleared_env(),
            )
        combined = _norm(proc.stdout + proc.stderr)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(_norm(ANDROID_NDK_MISSING), combined)

    def test_macos_toolchain_requires_osxcross_off_mac(self) -> None:
        if _host_system() == "Darwin":
            self.skipTest("osxcross is the non-Mac path")
        with tempfile.TemporaryDirectory(prefix="ce-host-macos-") as tmp:
            proc = _configure(
                Path(tmp) / "build",
                ["-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/macos.cmake", "-DBUILD_TESTS=OFF"],
                env=_cleared_env(),
            )
        combined = _norm(proc.stdout + proc.stderr)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(_norm(MACOS_OSXCROSS_MISSING), combined)

    def test_windows_mingw_toolchain(self) -> None:
        env = _cleared_env()
        with tempfile.TemporaryDirectory(prefix="ce-host-windows-") as tmp:
            proc = _configure(
                Path(tmp) / "build",
                [
                    "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake",
                    "-DBUILD_TESTS=OFF",
                    "-DBUILD_FFI=ON",
                ],
                env=env,
            )
        combined = _norm(proc.stdout + proc.stderr)
        if shutil.which("x86_64-w64-mingw32-g++") is None:
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn(_norm(WINDOWS_MINGW_MISSING), combined)
            return
        self.assertEqual(proc.returncode, 0, combined)
        self.assertIn("CoreEngine target platform: windows", combined)

    def test_android_rejects_tests_when_ndk_present(self) -> None:
        ndk = (
            os.environ.get("ANDROID_NDK")
            or os.environ.get("ANDROID_NDK_ROOT")
            or os.environ.get("ANDROID_NDK_HOME")
        )
        if not ndk:
            self.skipTest("ANDROID_NDK is not set")
        env = _cleared_env()
        env["ANDROID_NDK"] = ndk
        with tempfile.TemporaryDirectory(prefix="ce-host-android-tests-") as tmp:
            proc = _configure(
                Path(tmp) / "build",
                [
                    "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/android.cmake",
                    f"-DANDROID_NDK={ndk}",
                    "-DANDROID_ABI=arm64-v8a",
                    "-DBUILD_TESTS=ON",
                ],
                env=env,
            )
        combined = _norm(proc.stdout + proc.stderr)
        self.assertNotEqual(proc.returncode, 0, combined)
        self.assertIn(_norm(ANDROID_TESTS_REJECTED), combined)


if __name__ == "__main__":
    unittest.main()
