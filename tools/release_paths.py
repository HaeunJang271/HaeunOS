#!/usr/bin/env python3
"""Safe VERSION parsing, dist path confinement, and smoke suite identity."""
from __future__ import annotations

import os
import re
from pathlib import Path

_VERSION_RE = re.compile(r"^[0-9]+(?:\.[0-9]+)*(?:[a-zA-Z0-9][a-zA-Z0-9._-]*)?$")

# Single authority for mandatory product-smoke test binary stems (build + run).
SMOKE_TEST_STEMS: tuple[str, ...] = (
    "vm-tests",
    "hxf-tests",
    "bootinfo-tests",
    "hfw-tests",
    "hfs-tests",
    "proc-hfs-tests",
)
VM_BIN_STEM = "haeun-vm"


def validate_version(raw: str) -> str:
    """Return a safe VERSION string or abort on traversal/unsafe content."""
    if raw is None:
        raise SystemExit("refusing VERSION: empty")
    line = raw.replace("\r\n", "\n").replace("\r", "\n").split("\n", 1)[0].strip()
    if not line or any(ch.isspace() for ch in line):
        raise SystemExit("refusing VERSION: whitespace/empty")
    if any(ch in line for ch in ("/", "\\", "\0", "\t")) or ".." in line:
        raise SystemExit("refusing VERSION: path/control characters")
    if line.startswith(("/", "\\")) or re.match(r"^[A-Za-z]:[\\/]", line):
        raise SystemExit("refusing VERSION: absolute path")
    if not _VERSION_RE.fullmatch(line):
        raise SystemExit(f"refusing VERSION: unsafe format: {line!r}")
    return line


def read_version_file(root: Path) -> str:
    """Read and validate VERSION from a repository root."""
    path = root / "VERSION"
    if not path.is_file():
        raise SystemExit(f"refusing VERSION: missing {path}")
    return validate_version(path.read_text(encoding="utf-8"))


def ensure_under_directory(path: Path, parent: Path) -> Path:
    """Resolve path and require it to remain under parent."""
    parent_r = parent.resolve()
    path_r = path.resolve()
    try:
        path_r.relative_to(parent_r)
    except ValueError as exc:
        raise SystemExit(
            f"refusing path escape: {path_r} not under {parent_r}"
        ) from exc
    return path_r


def exe_suffix() -> str:
    """Return .exe on Windows hosts, else empty."""
    return ".exe" if os.name == "nt" else ""


def mandatory_smoke_bin_relpaths() -> list[str]:
    """Return repo-relative build/run paths for the six mandatory smoke bins."""
    suf = exe_suffix()
    return [f"build/{stem}{suf}" for stem in SMOKE_TEST_STEMS]


def mandatory_smoke_commands(
    *, root: Path, python_executable: str, make: str = "make"
) -> list[list[str]]:
    """Return the canonical mandatory smoke command argv lists (fixed order)."""
    root = root.resolve()
    commands = [[str(root / rel)] for rel in mandatory_smoke_bin_relpaths()]
    commands.append([make, "toolchain-test"])
    commands.append(
        [
            python_executable,
            str((root / "tests" / "m14_process_apps_tests.py").resolve()),
            str((root / f"build/{VM_BIN_STEM}{exe_suffix()}").resolve()),
        ]
    )
    return commands
