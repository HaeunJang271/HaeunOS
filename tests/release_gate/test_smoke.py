#!/usr/bin/env python3
"""Mandatory smoke suite and VERSION/path confinement regressions."""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

from fixtures import (
    REPO,
    expect_exit,
    fail_,
    init_git,
    install_smoke_stubs,
    pass_,
    require_gcc,
    session_for,
)

sys.path.insert(0, str(REPO / "tools"))
import release_gate as rg  # noqa: E402
import release_paths as rp  # noqa: E402


def test_smoke_build_and_run_lists_match() -> None:
    """Build targets and run argv lists share one authoritative stem list."""
    print("\n=== Smoke build/run list unification ===")
    bins = rp.mandatory_smoke_bin_relpaths()
    if rg.smoke_bin_names() != bins:
        fail_("smoke_bin_names", f"{rg.smoke_bin_names()} != {bins}")
    if len(bins) != len(rp.SMOKE_TEST_STEMS):
        fail_("stem count", f"{len(bins)} vs {len(rp.SMOKE_TEST_STEMS)}")
    for stem, rel in zip(rp.SMOKE_TEST_STEMS, bins, strict=True):
        expected = f"build/{stem}{rp.exe_suffix()}"
        if rel != expected:
            fail_("bin relpath", f"{rel} != {expected}")

    with tempfile.TemporaryDirectory(prefix="haeun-smoke-align-") as td:
        root = Path(td)
        (root / "VERSION").write_text("1.0.0\n", encoding="utf-8")
        cmds = rp.mandatory_smoke_commands(
            root=root, python_executable=sys.executable, make="make"
        )
        if len(cmds) != len(bins) + 2:
            fail_("command count", f"{len(cmds)} != {len(bins) + 2}")
        for rel, cmd in zip(bins, cmds[: len(bins)], strict=True):
            expected_cmd = [str(root / rel)]
            if cmd != expected_cmd:
                fail_("run argv vs build bin", f"{cmd} != {expected_cmd}")
        if cmds[len(bins)] != ["make", "toolchain-test"]:
            fail_("toolchain-test slot", str(cmds[len(bins)]))
        m14 = cmds[len(bins) + 1]
        if m14[0] != sys.executable:
            fail_("m14 python", str(m14))
        if not m14[1].endswith("m14_process_apps_tests.py"):
            fail_("m14 script", str(m14))
        vm = f"haeun-vm{rp.exe_suffix()}"
        if not m14[2].replace("\\", "/").endswith(f"build/{vm}"):
            fail_("m14 vm", str(m14))
    pass_("build targets match smoke run targets")


def test_smoke_command_list_enforcement() -> None:
    """Reject reduced/reordered/substituted/additional lists; seal full suite."""
    print("\n=== Mandatory smoke suite enforcement ===")
    with tempfile.TemporaryDirectory(prefix="haeun-smoke-") as td:
        repo = Path(td)
        head = init_git(repo)
        make_s, cmds = install_smoke_stubs(repo)
        s = session_for(repo)
        s.bind_head(head)

        expect_exit(
            lambda: s.run_checked_commands(
                cmds[:-1], root=repo, python_executable=sys.executable, make=make_s
            ),
            "reduced command list rejected",
        )
        expect_exit(
            lambda: s.run_checked_commands(
                list(reversed(cmds)),
                root=repo,
                python_executable=sys.executable,
                make=make_s,
            ),
            "reordered command list rejected",
        )
        substituted = [list(c) for c in cmds]
        substituted[0] = [sys.executable, "-c", "raise SystemExit(0)"]
        expect_exit(
            lambda: s.run_checked_commands(
                substituted, root=repo, python_executable=sys.executable, make=make_s
            ),
            "substituted command rejected",
        )
        additional = [list(c) for c in cmds] + [
            [sys.executable, "-c", "raise SystemExit(0)"]
        ]
        expect_exit(
            lambda: s.run_checked_commands(
                additional, root=repo, python_executable=sys.executable, make=make_s
            ),
            "additional command rejected",
        )

        s2 = session_for(repo, "ok")
        s2.bind_head(head)
        results = s2.run_checked_commands(
            cmds, root=repo, python_executable=sys.executable, make=make_s
        )
        if len(results) != len(cmds):
            fail_("complete suite", f"len={len(results)}")
        for item in results:
            if item.get("exit_status") != 0 or item.get("result") != "passed":
                fail_("complete suite status", str(item))
        pass_("complete successful sequence sealed")

        expect_exit(
            lambda: s2.run_checked_commands(
                cmds, root=repo, python_executable=sys.executable, make=make_s
            ),
            "second smoke invocation rejected",
        )

        s3 = session_for(repo, "fail")
        s3.bind_head(head)
        gcc = require_gcc()
        bad_c = repo / "stub_fail.c"
        bad_c.write_text("int main(void){return 1;}\n", encoding="utf-8")
        completed = subprocess.run(
            [gcc, "-O0", "-o", cmds[0][0], str(bad_c)],
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0:
            print(
                "ERROR: GCC failed compiling failing smoke stub:\n"
                f"{completed.stderr}",
                file=sys.stderr,
            )
            raise SystemExit(2)
        try:
            s3.run_checked_commands(
                cmds, root=repo, python_executable=sys.executable, make=make_s
            )
            fail_("failed command", "did not raise")
        except (SystemExit, OSError, subprocess.CalledProcessError):
            pass_("failed smoke command rejected")


def test_version_and_path_jail() -> None:
    """Reject unsafe VERSION values, VERSION mutation, and path escapes."""
    print("\n=== VERSION + path jail ===")
    expect_exit(lambda: rp.validate_version("../evil"), "VERSION .. rejected")
    expect_exit(lambda: rp.validate_version("1.0/hack"), "VERSION slash rejected")
    expect_exit(lambda: rp.validate_version("C:\\Windows"), "VERSION abs rejected")
    expect_exit(lambda: rp.validate_version("1.0.0 bad"), "VERSION whitespace rejected")
    if rp.validate_version("1.0.0") != "1.0.0":
        fail_("VERSION ok", "mismatch")
    pass_("safe VERSION accepted")

    with tempfile.TemporaryDirectory(prefix="haeun-ver-") as td:
        repo = Path(td)
        head = init_git(repo, "1.0.0")
        make_s, cmds = install_smoke_stubs(repo)
        s = session_for(repo)
        s.bind_head(head)
        s.run_checked_commands(
            cmds, root=repo, python_executable=sys.executable, make=make_s
        )
        (repo / "VERSION").write_text("9.9.9\n", encoding="utf-8")
        expect_exit(lambda: s.issue_authorization(), "VERSION mutation rejected")

    with tempfile.TemporaryDirectory(prefix="haeun-path-") as td:
        root = Path(td)
        dist = root / "dist"
        dist.mkdir()
        expect_exit(
            lambda: rp.ensure_under_directory(root / "outside", dist),
            "filesystem path escape rejected",
        )
        ok = rp.ensure_under_directory(dist / "haeun-os-1.0.0-abc", dist)
        try:
            ok.relative_to(dist.resolve())
        except ValueError:
            fail_("path under", str(ok))
        pass_("path confinement ok")
