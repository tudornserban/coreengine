#!/usr/bin/env python3
"""Validate relative markdown links in repo documentation."""
from __future__ import annotations

import re
import sys
from pathlib import Path

# [label](target) — skip images (![) and angle-bracket autolinks
LINK_RE = re.compile(r"(?<!!)\[([^\]]*)\]\(([^)]+)\)")

SKIP_SCHEMES = ("http://", "https://", "mailto:", "javascript:", "#")

DEFAULT_GLOBS = (
    "README.md",
    "AGENTS.md",
    "docs/**/*.md",
    "core/**/MODULE.md",
    "tools/**/MODULE.md",
)


def iter_doc_files(repo_root: Path) -> list[Path]:
    seen: set[Path] = set()
    out: list[Path] = []
    for pattern in DEFAULT_GLOBS:
        for path in sorted(repo_root.glob(pattern)):
            if path.is_file():
                resolved = path.resolve()
                if resolved not in seen:
                    seen.add(resolved)
                    out.append(path)
    return out


def resolve_link(source: Path, target: str, repo_root: Path) -> tuple[Path | None, str]:
    """Return (path_to_check, anchor) for a markdown link target."""
    target = target.strip()
    if not target or target.startswith(SKIP_SCHEMES):
        return None, ""
    if target.startswith("#"):
        return None, ""

    fragment = ""
    if "#" in target:
        target, fragment = target.split("#", 1)

    if target.startswith("/"):
        candidate = (repo_root / target.lstrip("/")).resolve()
    else:
        candidate = (source.parent / target).resolve()

    return candidate, fragment


def strip_fenced_code(text: str) -> str:
    """Remove fenced and inline-code segments so examples are not scanned as links."""
    out: list[str] = []
    in_fence = False
    for line in text.splitlines():
        if line.strip().startswith("```"):
            in_fence = not in_fence
            out.append("")
            continue
        if in_fence:
            out.append("")
            continue
        out.append(re.sub(r"`[^`]*`", "", line))
    return "\n".join(out)


def check_file(path: Path, repo_root: Path) -> list[str]:
    errors: list[str] = []
    try:
        text = strip_fenced_code(path.read_text(encoding="utf-8"))
    except OSError as exc:
        return [f"{path.relative_to(repo_root)}: cannot read: {exc}"]

    for line_no, line in enumerate(text.splitlines(), start=1):
        for match in LINK_RE.finditer(line):
            raw = match.group(2).strip()
            resolved, _anchor = resolve_link(path, raw, repo_root)
            if resolved is None:
                continue
            rel = path.relative_to(repo_root)
            if not resolved.exists():
                errors.append(
                    f"{rel}:{line_no}: broken link {raw!r} "
                    f"(resolved {resolved.relative_to(repo_root) if resolved.is_relative_to(repo_root) else resolved})"
                )
    return errors


def run_checks(repo_root: Path) -> tuple[bool, list[str]]:
    errors: list[str] = []
    for doc in iter_doc_files(repo_root):
        errors.extend(check_file(doc, repo_root))
    return (len(errors) == 0, errors)


def main(argv: list[str] | None = None) -> int:
    repo_root = Path(__file__).resolve().parents[1]
    ok, errors = run_checks(repo_root)
    if not ok:
        print("FAIL: documentation link check")
        for err in errors:
            print(f"  - {err}")
        return 1
    n = len(iter_doc_files(repo_root))
    print(f"OK: documentation links ({n} markdown files scanned).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
