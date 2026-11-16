#!/usr/bin/env python3
"""M22.4: rendering stability — bounds, tile gaps, life-cycle restore (no OCR)."""
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
    """Return RGB at (x,y) from binary P6 PPM (320×180)."""
    assert 0 <= x < FB_W and 0 <= y < FB_H, (x, y)
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    dims = parts[1].decode().split()
    assert dims == [str(FB_W), str(FB_H)], dims
    body = parts[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


def run_dump(vm: Path, desk: Path, ppm: Path, extra: list[str]) -> str:
    """Run desktop with --fb-dump; return combined log."""
    r = subprocess.run(
        [str(vm), str(desk), "--apps", "--persist", *extra, "--fb-dump", str(ppm)],
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert ppm.is_file(), blob
    assert "wrote framebuffer PPM" in blob, blob
    assert "[haeun-fault]" not in blob, blob
    return blob


def tile_accent(path: Path) -> tuple[tuple[int, int, int], ...]:
    """Sample the three tile badge pixels (origin+8,+8)."""
    return (
        read_ppm_rgb(path, 24, 96),
        read_ppm_rgb(path, 120, 96),
        read_ppm_rgb(path, 216, 96),
    )


def assert_idle_structure(path: Path) -> None:
    """Bounds, three distinct tiles, wallpaper gaps, taskbar."""
    # Corners must be readable in-FB colors (header / taskbar), not crash.
    for x, y in ((0, 0), (FB_W - 1, 0), (0, FB_H - 1), (FB_W - 1, FB_H - 1)):
        rgb = read_ppm_rgb(path, x, y)
        assert sum(rgb) < 200, (x, y, rgb, "edge should be dark chrome/bar")

    c, e, f = tile_accent(path)
    assert c[1] > 60, (c, "calc accent")
    assert e[1] > 40 or e[2] > 40, (e, "edit accent")
    assert f[0] > 40 or f[2] > 40, (f, "files accent")
    # Accents must differ — catches duplicate/same-tile paint.
    assert c != e and e != f and c != f, (c, e, f)

    gap1 = read_ppm_rgb(path, 108, 100)
    gap2 = read_ppm_rgb(path, 204, 100)
    assert gap1 != c and gap1 != e, (gap1, "gap between calc/edit")
    assert gap2 != e and gap2 != f, (gap2, "gap between edit/files")
    assert sum(gap1) < 200 and sum(gap2) < 200, (gap1, gap2)

    # Past Files tile right edge (tile ends at x=304).
    past = read_ppm_rgb(path, 310, 100)
    assert past != f and sum(past) < 200, (past, "past files tile")

    bar = read_ppm_rgb(path, 160, 170)
    assert sum(bar) < 120, (bar, "taskbar")


def main(argv: list[str]) -> int:
    """M22.4 rendering acceptance (structural; OCR left to m22.3)."""
    if len(argv) != 2:
        print("usage: m22_4_rendering_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())

    idle = out / "m22_4_idle.ppm"
    run_dump(vm, desk, idle, ["--max-steps", "2000000"])
    assert_idle_structure(idle)

    # App open: calc window fills mid region; shell corners still intact.
    calc_ppm = out / "m22_4_calc_open.ppm"
    run_dump(
        vm,
        desk,
        calc_ppm,
        ["--input-script", "calc-add", "--max-steps", "300000"],
    )
    assert sum(read_ppm_rgb(calc_ppm, 0, 0)) < 200
    # Calc client fill near (160,80) — window (72,36)+(88,44).
    mid = read_ppm_rgb(calc_ppm, 160, 80)
    assert mid == (32, 24, 16) or sum(mid) < 120, (mid, "calc client/backdrop")

    edit_ppm = out / "m22_4_edit_open.ppm"
    run_dump(
        vm,
        desk,
        edit_ppm,
        ["--input-script", "edit-type", "--max-steps", "300000"],
    )
    assert sum(read_ppm_rgb(edit_ppm, 160, 90)) > 0

    files_ppm = out / "m22_4_files_open.ppm"
    blob = run_dump(
        vm,
        desk,
        files_ppm,
        ["--input-script", "files-list", "--max-steps", "300000"],
    )
    assert "app-env: spawn files" in blob

    # Switch / repeat: life-cycle then desktop must match idle structure.
    life = out / "m22_4_after_life.ppm"
    blob = run_dump(
        vm,
        desk,
        life,
        ["--input-script", "life-cycle", "--max-steps", "800000"],
    )
    assert "guest halted" in blob and "r1=0x00000000" in blob, blob
    assert_idle_structure(life)
    assert tile_accent(life) == tile_accent(idle), (
        tile_accent(life),
        tile_accent(idle),
        "tiles changed after life-cycle",
    )

    print("m22_4_rendering_tests: PASS")
    print(f"screenshots: {idle.name}, {calc_ppm.name}, {edit_ppm.name}, "
          f"{files_ppm.name}, {life.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
