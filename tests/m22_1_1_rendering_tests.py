#!/usr/bin/env python3
"""M22.1.1: fullscreen shell render + no host chrome on backdrop."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402

FB_W, FB_H = 320, 180


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
    """Return RGB at (x,y) from binary P6 PPM."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    body = parts[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


def black_count(path: Path) -> int:
    """Count pure-black pixels in a 64x32 PPM."""
    body = path.read_bytes().split(b"\n", 3)[3]
    return sum(
        1 for i in range(0, len(body), 3) if body[i] | body[i + 1] | body[i + 2] == 0
    )


def main(argv: list[str]) -> int:
    """Idle desktop must be full-bleed without host chrome title/buttons."""
    if len(argv) != 2:
        print("usage: m22_1_1_rendering_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    ppm = out / "m22_1_1_idle_after.ppm"
    r = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--max-steps",
            "8000000",
            "--fb-dump",
            str(ppm),
        ],
        capture_output=True,
        text=True,
        check=False,
        cwd=str(ROOT),
        timeout=180,
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert ppm.is_file(), blob
    assert "wrote framebuffer PPM" in r.stdout, blob

    y0 = read_ppm_rgb(ppm, 160, 2)
    btn = read_ppm_rgb(ppm, 310, 2)
    ico = read_ppm_rgb(ppm, 24, 96)
    bar = read_ppm_rgb(ppm, 160, 170)
    mid = read_ppm_rgb(ppm, 160, 80)
    assert sum(y0) < 120, (y0, "no host chrome title band")
    assert not (btn[0] > 200 and btn[1] < 80 and btn[2] < 80), (
        btn,
        "no host close button",
    )
    assert ico[1] > 60 and ico[1] >= ico[0], (ico, "calc tile greenish")
    assert sum(bar) < 120, (bar, "taskbar dark")
    assert sum(mid) < 200, (mid, "wallpaper")
    assert black_count(ppm) < 512, "desktop must not be mostly black"

    print("m22_1_1_rendering_tests: PASS")
    print(f"screenshot: {ppm}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
