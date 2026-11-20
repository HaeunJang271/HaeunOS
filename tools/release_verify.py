#!/usr/bin/env python3
"""Independent package manifest, checksum, and provenance writers."""
from __future__ import annotations

import hashlib
import json
import os
import platform
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def sha256_file(path: Path) -> str:
    """Return hex SHA-256 of a file."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def assert_manifest_and_sums(root: Path) -> None:
    """Verify SHA256SUMS and manifest coverage against actual file bytes."""
    sums_path = root / "SHA256SUMS"
    man_path = root / "PACKAGE_MANIFEST.json"
    if not sums_path.is_file() or not man_path.is_file():
        raise SystemExit("refusing package: missing manifest or SHA256SUMS")
    listed: set[str] = set()
    for line in sums_path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        digest, rel = line.split(None, 1)
        rel = rel.strip()
        if rel == "SHA256SUMS":
            raise SystemExit("refusing package: SHA256SUMS checksums itself")
        path = root / rel
        if not path.is_file():
            raise SystemExit(f"refusing package: checksum entry missing file {rel}")
        if sha256_file(path) != digest:
            raise SystemExit(f"refusing package: checksum mismatch for {rel}")
        listed.add(rel)
    if "BUILD_PROVENANCE.json" not in listed or "PACKAGE_MANIFEST.json" not in listed:
        raise SystemExit("refusing package: provenance/manifest not in SHA256SUMS")
    man = json.loads(man_path.read_text(encoding="utf-8"))
    man_paths = {f["path"] for f in man.get("files", [])}
    if "PACKAGE_MANIFEST.json" in man_paths or "SHA256SUMS" in man_paths:
        raise SystemExit("refusing package: manifest lists self or SHA256SUMS")
    if "BUILD_PROVENANCE.json" not in man_paths:
        raise SystemExit("refusing package: manifest missing provenance")
    disk_files = {
        p.relative_to(root).as_posix()
        for p in root.rglob("*")
        if p.is_file() and p.name != "SHA256SUMS"
    }
    expected_manifest = disk_files - {"PACKAGE_MANIFEST.json"}
    if man_paths != expected_manifest:
        raise SystemExit("refusing package: manifest coverage mismatch")
    if listed != disk_files:
        raise SystemExit("refusing package: SHA256SUMS coverage mismatch")


def _git(repo_root: Path, argv: list[str]) -> str:
    """Return stripped git stdout from repo_root."""
    return subprocess.check_output(
        ["git", *argv], cwd=str(repo_root), text=True
    ).strip()


def _tool_version(argv: list[str]) -> str:
    """Return first line of a version command or unknown."""
    try:
        out = subprocess.check_output(
            argv, text=True, stderr=subprocess.STDOUT
        )
        return out.splitlines()[0].strip() if out.strip() else "unknown"
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def write_provenance(
    root: Path,
    *,
    repo_root: Path,
    package_name: str,
    ver: str,
    smoke_status: str,
    official_entry_point: str,
    build_commands: list[dict[str, Any]],
    smoke_results: list[dict[str, Any]] | None,
) -> None:
    """Write BUILD_PROVENANCE.json from verified execution records only."""
    head = _git(repo_root, ["rev-parse", "HEAD"])
    short = _git(repo_root, ["rev-parse", "--short", "HEAD"])
    dirty = bool(_git(repo_root, ["status", "--porcelain"]))
    try:
        tag = subprocess.check_output(
            ["git", "describe", "--exact-match", "--tags", "HEAD"],
            cwd=str(repo_root),
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except subprocess.CalledProcessError:
        tag = None
    cc = os.environ.get("CC", "cc")
    smoke = list(smoke_results or [])
    doc = {
        "schema_version": "1",
        "package_name": package_name,
        "version": ver,
        "git_commit": head,
        "git_commit_short": short,
        "git_tag": tag,
        "git_dirty": dirty,
        "built_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "os": platform.system(),
        "architecture": platform.machine(),
        "compiler": cc,
        "compiler_version": _tool_version([cc, "--version"]),
        "python_version": sys.version.split("\n")[0],
        "make_version": _tool_version(["make", "--version"]),
        "official_entry_point": official_entry_point,
        "build_commands": list(build_commands),
        "smoke_commands": smoke,
        "smoke_test_status": smoke_status,
        "smoke_results": smoke,
        "reproducible_bit_identical": False,
        "notes": (
            "Records source commit, environment, and actual executed commands; "
            "does not claim bit-identical binary reproduction. "
            "Authorization is not a same-process adversarial isolation boundary."
        ),
    }
    (root / "BUILD_PROVENANCE.json").write_text(
        json.dumps(doc, indent=2) + "\n", encoding="utf-8"
    )
