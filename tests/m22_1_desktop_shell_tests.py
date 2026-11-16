#!/usr/bin/env python3
"""M22.1: desktop shell render + M21 launch/persist regression."""
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

FB_W = 320
FB_H = 180
# Guest colors are 0x00BBGGRR style in practice for PPM (see smoke sample).
# Prefer structural checks: dark chrome vs green icon vs darker taskband.


def build_apps(out: Path) -> None:
    """Build builtin --apps images."""
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


def read_ppm_rgb(path: Path, x: int, y: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from binary P6 PPM (320×180 shell FB)."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    body = parts[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


def run_vm(vm: Path, args: list[str], timeout: int = 300) -> tuple[int, str, str]:
    """Run haeun-vm."""
    r = subprocess.run(
        [str(vm), *args],
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(ROOT),
    )
    return r.returncode, r.stdout or "", r.stderr or ""


def main(argv: list[str]) -> int:
    """M22.1 shell acceptance."""
    if len(argv) != 2:
        print("usage: m22_1_desktop_shell_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    ppm = out / "m22_1_shell.ppm"

    # Init + paint + launches + Esc (M21 script); dump FB.
    code, so, se = run_vm(
        vm,
        [
            str(desk),
            "--apps",
            "--persist",
            "--input",
            "--fb-dump",
            str(ppm),
        ],
    )
    blob = so + se
    assert code == 0, blob
    assert "guest halted" in so, blob
    assert "r1=0x00000000" in so, blob
    assert "app-env: spawn calc" in so, blob
    assert "app-env: spawn edit" in so, blob
    assert ppm.is_file(), "missing shell PPM"

    # Fullscreen 320x180 shell; no host chrome title band.
    top = read_ppm_rgb(ppm, 160, 2)
    ico = read_ppm_rgb(ppm, 24, 96)
    bar = read_ppm_rgb(ppm, 160, 170)
    mid = read_ppm_rgb(ppm, 160, 80)
    assert sum(top) < 120, (top, "top status should be dark")
    assert ico[1] >= ico[0] and ico[1] >= ico[2] and ico[1] > 60, (
        ico,
        "icon should be greenish",
    )
    assert sum(bar) < 120, (bar, "taskbar should be dark")
    assert sum(mid) < 200, (mid, "desktop background")
    black = 0
    for y in range(0, FB_H, 4):
        for x in range(0, FB_W, 4):
            if read_ppm_rgb(ppm, x, y) == (0, 0, 0):
                black += 1
    assert black < 200, (black, "desktop should not be mostly black")

    # Persist past historic 10M: ceiling at 11M must be reachable (no early stop).
    code, so, se = run_vm(
        vm,
        [str(desk), "--apps", "--persist", "--max-steps", "11000000"],
        timeout=180,
    )
    blob = so + se
    assert code != 0, blob
    assert "STEP_LIMIT" in se, blob
    assert re.search(r"after 11000000 instructions", se), blob

    # M20 launcher regression.
    (out / "process_launch.hxf").write_bytes(build_process_launch.build())
    code, so, se = run_vm(vm, [str(out / "process_launch.hxf"), "--apps"])
    assert code == 0 and "guest halted" in so, so + se

    print("m22_1_desktop_shell_tests: PASS")
    print(f"screenshot: {ppm}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
