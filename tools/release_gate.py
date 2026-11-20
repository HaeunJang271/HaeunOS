#!/usr/bin/env python3
"""Sequential provenance-aware product release gate (no make test)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from release_auth import GateSession  # noqa: E402
from release_paths import (  # noqa: E402
    mandatory_smoke_bin_relpaths,
    mandatory_smoke_commands,
)


def _make_cmd() -> str:
    """Resolve make executable name."""
    return "make"


def run_checked(
    argv: list[str], *, cwd: Path = ROOT, session: GateSession | None = None
) -> None:
    """Run argv with check=True; record via session only after real success."""
    if session is not None:
        session.run_and_record_build(list(argv))
        return
    print("+", " ".join(argv), flush=True)
    subprocess.run(argv, cwd=str(cwd), check=True)


def git_output(argv: list[str]) -> str:
    """Return stdout from a git command in ROOT."""
    r = subprocess.run(
        ["git", *argv],
        cwd=str(ROOT),
        check=True,
        capture_output=True,
        text=True,
    )
    return r.stdout


def validate_git_clean(*, phase: str) -> str:
    """
    Reject staged, tracked dirty, or untracked non-ignored files.

    Returns the full HEAD SHA when clean.
    """
    branch = git_output(["rev-parse", "--abbrev-ref", "HEAD"]).strip()
    head = git_output(["rev-parse", "HEAD"]).strip()
    print(f"git[{phase}]: branch={branch} HEAD={head}", flush=True)
    staged = [
        ln
        for ln in git_output(["diff", "--cached", "--name-only"]).splitlines()
        if ln.strip()
    ]
    tracked_dirty = [
        ln for ln in git_output(["diff", "--name-only"]).splitlines() if ln.strip()
    ]
    untracked = [
        ln
        for ln in git_output(["ls-files", "--others", "--exclude-standard"]).splitlines()
        if ln.strip()
    ]
    reasons: list[str] = []
    if staged:
        reasons.append("staged changes: " + ", ".join(staged))
    if tracked_dirty:
        reasons.append("tracked modifications: " + ", ".join(tracked_dirty))
    if untracked:
        reasons.append("untracked non-ignored files: " + ", ".join(untracked))
    if reasons:
        print(f"REJECT git[{phase}]:", file=sys.stderr)
        for r in reasons:
            print(f"  - {r}", file=sys.stderr)
        raise SystemExit(2)
    print(f"git[{phase}]: clean", flush=True)
    return head


def smoke_bin_names() -> list[str]:
    """Return relative paths of the six mandatory smoke binaries (shared authority)."""
    return mandatory_smoke_bin_relpaths()


def new_gate_session(*, official_entry_point: str) -> GateSession:
    """Construct a GateSession bound to this module's ROOT git helper."""
    return GateSession(
        official_entry_point=official_entry_point,
        git_output=git_output,
        cwd=ROOT,
    )


def run_smoke(session: GateSession) -> list[dict[str, Any]]:
    """Build mandatory smoke binaries, then seal the canonical command suite."""
    make = _make_cmd()
    bins = smoke_bin_names()
    run_checked([make, *bins], session=session)
    commands = mandatory_smoke_commands(
        root=ROOT, python_executable=sys.executable, make=make
    )
    return session.run_checked_commands(
        commands, root=ROOT, python_executable=sys.executable, make=make
    )


def main(argv: list[str] | None = None) -> int:
    """
    Run the sequential release gate and provenanced package step.

    Does not invoke make test. Aborts on dirty git or any command failure.
    """
    del argv
    import package_release as pr  # noqa: WPS001

    head_before = validate_git_clean(phase="pre-build")
    make = _make_cmd()
    session = new_gate_session(official_entry_point="make release-provenanced")
    session.bind_head(head_before)
    run_checked([make, "clean"], session=session)
    run_checked([make, "all"], session=session)
    run_smoke(session)
    head_after = validate_git_clean(phase="post-smoke")
    if head_after != head_before:
        print(
            f"REJECT: HEAD changed during gate ({head_before} -> {head_after})",
            file=sys.stderr,
        )
        return 2
    session.bind_head(head_after)
    auth = session.issue_authorization()
    dest = pr.package_provenanced(session=session, authorization=auth)
    print(f"release-gate OK: package at {dest}", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"FAIL command exit={exc.returncode}: {exc.cmd}", file=sys.stderr)
        raise SystemExit(exc.returncode or 1) from exc
