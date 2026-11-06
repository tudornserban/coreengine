"""Guardrails for removing repo-local CMake build directories."""
from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path


def resolve_build_dir(repo_root: Path, build_dir: Path | str) -> Path:
    repo = repo_root.resolve()
    if build_dir == "" or (isinstance(build_dir, str) and not str(build_dir).strip()):
        raise ValueError("empty build directory path")
    candidate = Path(build_dir)
    if not candidate.is_absolute():
        candidate = repo / candidate
    try:
        resolved = candidate.resolve(strict=False)
    except (OSError, RuntimeError) as exc:
        raise ValueError(f"cannot resolve build directory {candidate!r}: {exc}") from exc

    if resolved == repo:
        raise ValueError("refusing repository root")
    if resolved == repo.parent or resolved == Path("/"):
        raise ValueError(f"refusing unsafe path {resolved}")

    try:
        rel = resolved.relative_to(repo)
    except ValueError as exc:
        raise ValueError(f"build directory must be inside repo root {repo}: {resolved}") from exc

    if len(rel.parts) != 1:
        raise ValueError(
            f"build directory must be a direct child of the repo root, got {rel.as_posix()!r}"
        )

    name = rel.parts[0]
    if not name.startswith("build-"):
        raise ValueError(f"build directory name must start with 'build-', got {name!r}")

    if os.path.realpath(resolved.parent) != os.path.realpath(repo):
        raise ValueError("refusing path whose parent does not match repo root (symlink escape)")

    return resolved


def safe_rm_build_dir(repo_root: Path, build_dir: Path | str) -> Path:
    """Delete a validated repo-local build-* directory. Returns resolved path."""
    target = resolve_build_dir(repo_root, build_dir)
    if target.exists() or target.is_symlink():
        shutil.rmtree(target)
    return target


def main(argv: list[str] | None = None) -> int:
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 2:
        print("usage: build_dir_guard.py <repo-root> <build-dir>", file=sys.stderr)
        return 2
    repo_root = Path(args[0])
    build_dir = args[1]
    try:
        removed = safe_rm_build_dir(repo_root, build_dir)
    except ValueError as exc:
        print(f"build_dir_guard: rejected: {exc}", file=sys.stderr)
        return 1
    print(f"build_dir_guard: removed {removed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())