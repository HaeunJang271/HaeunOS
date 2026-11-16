#!/usr/bin/env python3
"""M22.1.3+: 320x180 desktop layout — header, welcome, tiles, taskbar."""
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
    body = path.read_bytes().split(b"\n", 3)[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


def bright(rgb: tuple[int, int, int]) -> bool:
    """True if pixel looks like light glyph ink."""
    return sum(rgb) > 300 and max(rgb) > 140


def main(argv: list[str]) -> int:
    """Idle desktop matches target composition; launch still works."""
    if len(argv) != 2:
        print("usage: m22_1_2_composition_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    ppm = out / "m22_1_2_idle.ppm"

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
    assert "wrote framebuffer PPM" in (r.stdout or ""), blob
    assert "app-env: spawn" not in (r.stdout or ""), "idle must not auto-spawn"

    # Wallpaper / panel regions
    mid = read_ppm_rgb(ppm, 160, 80)
    assert 20 < sum(mid) < 200, (mid, "desktop mid")

    assert any(
        bright(read_ppm_rgb(ppm, x, y)) for x in range(8, 80) for y in range(4, 16)
    ), "HAEUN-OS title"

    # Welcome panel fill
    wel = read_ppm_rgb(ppm, 40, 40)
    assert sum(wel) > 40, (wel, "welcome panel")

    # Three tile accent badges (origin+8,+8)
    c = read_ppm_rgb(ppm, 24, 96)
    e = read_ppm_rgb(ppm, 120, 96)
    f = read_ppm_rgb(ppm, 216, 96)
    assert c[1] > 60, (c, "calc tile")
    assert e[1] > 40 or e[2] > 40, (e, "edit tile")
    assert f[0] > 40 or f[2] > 40, (f, "file tile")

    # Tile labels under badges
    assert any(bright(read_ppm_rgb(ppm, x, 124)) for x in range(24, 100)), "Calculator"
    assert any(bright(read_ppm_rgb(ppm, x, 124)) for x in range(120, 200)), "Editor"

    bar = read_ppm_rgb(ppm, 160, 170)
    assert sum(bar) < 120, (bar, "taskbar")
    assert any(bright(read_ppm_rgb(ppm, x, 170)) for x in range(8, 60)), "HAEUN footer"

    r = subprocess.run(
        [str(vm), str(desk), "--apps", "--persist", "--input"],
        capture_output=True,
        text=True,
        check=False,
        cwd=str(ROOT),
        timeout=300,
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "app-env: spawn calc" in (r.stdout or ""), blob
    assert "guest halted" in (r.stdout or ""), blob

    print("m22_1_2_composition_tests: PASS")
    print(f"screenshot: {ppm}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
