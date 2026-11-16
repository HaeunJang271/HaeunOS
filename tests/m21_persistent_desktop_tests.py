#!/usr/bin/env python3
"""M21: persistent desktop — idle, launch/reap, NOSPC, Esc shutdown, M20 gate."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402
import build_process_launch  # noqa: E402


def build_apps(out: Path) -> None:
    """Build builtin --apps images required by the host registry."""
    for name, src, extras in (
        ("app_calc.hxf", "app_calc.s", []),
        ("app_edit.hxf", "app_edit.s", []),
        ("app_files.hxf", "app_files.s", []),
        ("app_svc_idle.hxf", "app_svc_idle.s", []),
        ("app_filemgr.hxf", "app_filemgr.s", [ROOT / "userland" / "libapp.s"]),
    ):
        (out / name).write_bytes(
            build_app_hxf.build_app(ROOT / "userland" / src, extras)
        )


def run_desktop(vm: Path, out: Path, extra: list[str] | None = None) -> tuple[int, str, str]:
    """Run process_desktop with --apps --persist --input."""
    hxf = out / "process_desktop.hxf"
    hxf.write_bytes(build_process_desktop.build())
    cmd = [
        str(vm),
        str(hxf),
        "--apps",
        "--persist",
        "--input",
        *(extra or []),
    ]
    r = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    return r.returncode, r.stdout or "", r.stderr or ""


def halted_steps(stdout: str) -> int:
    """Parse instruction count from guest halted line."""
    m = re.search(r"guest halted after (\d+) instructions", stdout)
    assert m, stdout
    return int(m.group(1))


def main(argv: list[str]) -> int:
    """Execute M21 automated acceptance checks."""
    if len(argv) != 2:
        print("usage: m21_persistent_desktop_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)

    # 1–12: single persist script covers start/idle/apps/repeat/NOSPC/input/shutdown
    ppm = out / "m21_desktop.ppm"
    code, so, se = run_desktop(vm, out, ["--fb-dump", str(ppm)])
    blob = so + se
    if code != 0:
        print(blob, file=sys.stderr)
        return 1

    assert "guest halted" in so, blob
    assert "[haeun-fault]" not in blob, blob
    steps = halted_steps(so)
    # Esc is scripted at 700000 — proves idle/desktop survived until then.
    assert steps >= 700000, (steps, "desktop halted before Esc timeline")

    assert "app-env: spawn calc" in so, blob
    assert "app-env: spawn edit" in so, blob
    assert "app-env: spawn files" in so, blob
    # Repeated calc launch (key 1 twice in script).
    assert so.count("app-env: spawn calc") >= 2, blob
    assert "app-env: spawn NOSPC" in blob, blob

    # Keyboard reached desktop owner (Esc / digit make via evt route).
    assert re.search(r"\[haeun-evt\] key pid=\d+ sc=0x0[1-5] down", blob, re.I), blob
    # Mouse focus path exercised (scripted click + focus_hits / chrome).
    assert re.search(r"input-host:.*mice=[1-9]", blob) or re.search(
        r"focus_hits=[1-9]", blob
    ), blob

    assert "r1=0x00000000" in so, blob  # EXIT(0) shutdown
    assert ppm.is_file(), "missing M21 PPM"

    # 13: M20 launcher regression still builds and clean-halts.
    (out / "process_launch.hxf").write_bytes(build_process_launch.build())
    r = subprocess.run(
        [str(vm), str(out / "process_launch.hxf"), "--apps"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    m20 = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, m20
    assert "guest halted" in (r.stdout or ""), m20
    assert "app-env: spawn calc" in (r.stdout or ""), m20
    assert "[haeun-fault]" not in m20, m20

    print("m21_persistent_desktop_tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
