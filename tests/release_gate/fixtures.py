#!/usr/bin/env python3
"""Shared fixtures for release-gate authorization regression tests."""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))

import release_auth as ra  # noqa: E402
import release_paths as rp  # noqa: E402


def pass_(name: str) -> None:
    """Print a PASS line."""
    print(f"PASS: {name}", flush=True)


def fail_(name: str, detail: str) -> None:
    """Abort the suite with a FAIL line."""
    print(f"FAIL: {name}: {detail}", flush=True)
    raise SystemExit(1)


def expect_exit(fn, name: str) -> None:
    """Require SystemExit or TypeError from fn."""
    try:
        fn()
        fail_(name, "did not raise SystemExit/TypeError")
    except (SystemExit, TypeError):
        pass_(name)


def require_gcc() -> str:
    """
    Return the gcc executable path, or abort with a clear error.

    release-gate-test compiles C stubs for mandatory smoke binaries; without
    GCC the suite must fail closed (never silent PASS / skip).
    """
    gcc = shutil.which("gcc")
    if not gcc:
        print(
            "ERROR: release-gate-test requires GCC (`gcc`) on PATH "
            "to compile fixture smoke stubs. Install a C compiler "
            "and re-run `make release-gate-test`.",
            file=sys.stderr,
        )
        raise SystemExit(2)
    return gcc


def init_git(repo: Path, version: str = "1.0.0") -> str:
    """Initialize an isolated git repo fixture and return HEAD SHA."""
    subprocess.run(["git", "init"], cwd=str(repo), check=True, capture_output=True)
    subprocess.run(
        ["git", "config", "user.email", "gate@test.local"], cwd=str(repo), check=True
    )
    subprocess.run(
        ["git", "config", "user.name", "Gate Test"], cwd=str(repo), check=True
    )
    (repo / "VERSION").write_text(version + "\n", encoding="utf-8")
    (repo / "README.md").write_text("fixture\n", encoding="utf-8")
    (repo / ".gitignore").write_text(
        "build/\ndist/\ntests/\nfake_make.*\nfake_build_make.*\nstub_*.c\n",
        encoding="utf-8",
    )
    subprocess.run(
        ["git", "add", "VERSION", "README.md", ".gitignore"], cwd=str(repo), check=True
    )
    subprocess.run(
        ["git", "commit", "-m", "init"], cwd=str(repo), check=True, capture_output=True
    )
    return subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=str(repo), text=True
    ).strip()


def git_for(repo: Path):
    """Return a git_output callable bound to repo."""

    def git_output(argv: list[str]) -> str:
        r = subprocess.run(
            ["git", *argv],
            cwd=str(repo),
            check=True,
            capture_output=True,
            text=True,
        )
        return r.stdout

    return git_output


def session_for(
    repo: Path, official_entry_point: str = "make release-provenanced"
) -> ra.GateSession:
    """Create a GateSession for an isolated fixture repo."""
    return ra.GateSession(
        official_entry_point=official_entry_point,
        git_output=git_for(repo),
        cwd=repo,
    )


def install_smoke_stubs(repo: Path) -> tuple[str, list[list[str]]]:
    """Install stub binaries so the mandatory suite can execute in fixtures."""
    gcc = require_gcc()
    suf = rp.exe_suffix()
    build = repo / "build"
    build.mkdir(parents=True, exist_ok=True)
    (repo / "tests").mkdir(parents=True, exist_ok=True)
    (repo / "tests" / "m14_process_apps_tests.py").write_text(
        "raise SystemExit(0)\n", encoding="utf-8"
    )
    stub_c = repo / "stub_ok.c"
    stub_c.write_text("int main(void){return 0;}\n", encoding="utf-8")
    # Build targets must match mandatory_smoke_bin_relpaths + VM binary.
    names = [f"{stem}{suf}" for stem in rp.SMOKE_TEST_STEMS]
    names.append(f"{rp.VM_BIN_STEM}{suf}")
    for name in names:
        completed = subprocess.run(
            [gcc, "-O0", "-o", str(build / name), str(stub_c)],
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0:
            print(
                "ERROR: GCC failed compiling release-gate fixture stub "
                f"{name}:\n{completed.stderr}",
                file=sys.stderr,
            )
            raise SystemExit(2)
    make_py = repo / "fake_make.py"
    make_py.write_text(
        "import sys\n"
        "raise SystemExit(0 if sys.argv[1:] == ['toolchain-test'] else 1)\n",
        encoding="utf-8",
    )
    if os.name == "nt":
        make = repo / "fake_make.cmd"
        make.write_text(
            f"@echo off\r\n\"{sys.executable}\" \"{make_py}\" %*\r\n",
            encoding="utf-8",
        )
        make_s = str(make)
    else:
        make = repo / "fake_make.sh"
        make.write_text(
            f"#!/bin/sh\nexec '{sys.executable}' '{make_py}' \"$@\"\n",
            encoding="utf-8",
        )
        make.chmod(0o755)
        make_s = str(make)
    cmds = rp.mandatory_smoke_commands(
        root=repo, python_executable=sys.executable, make=make_s
    )
    return make_s, cmds


def install_build_recorder(repo: Path) -> str:
    """
    Install a fake make that accepts clean/all for real subprocess recording.

    Returns the make argv[0] path to pass to GateSession.run_and_record_build.
    """
    make_py = repo / "fake_build_make.py"
    make_py.write_text(
        "import sys\n"
        "ok = sys.argv[1:] in (['clean'], ['all'])\n"
        "raise SystemExit(0 if ok else 1)\n",
        encoding="utf-8",
    )
    if os.name == "nt":
        make = repo / "fake_build_make.cmd"
        make.write_text(
            f"@echo off\r\n\"{sys.executable}\" \"{make_py}\" %*\r\n",
            encoding="utf-8",
        )
        return str(make)
    make = repo / "fake_build_make.sh"
    make.write_text(
        f"#!/bin/sh\nexec '{sys.executable}' '{make_py}' \"$@\"\n",
        encoding="utf-8",
    )
    make.chmod(0o755)
    return str(make)


def seal_session(
    repo: Path, s: ra.GateSession
) -> tuple[list[dict], str, list[list[str]]]:
    """Run the mandatory stub suite; return results, make path, commands."""
    make_s, cmds = install_smoke_stubs(repo)
    results = s.run_checked_commands(
        cmds, root=repo, python_executable=sys.executable, make=make_s
    )
    return results, make_s, cmds
