#!/usr/bin/env python3
"""M23.4: composition stability (host pixels + guest PPM + desktop dump)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402
import build_process_sdk  # noqa: E402

FB_W = 320


def read_ppm_rgb(path: Path, x: int, y: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from binary P6 PPM."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    body = parts[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


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


def main(argv: list[str]) -> int:
    """Host compose checks + guest z-order PPM + desktop raise/min dump."""
    if len(argv) != 2:
        print("usage: m23_4_composition_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)

    unit = out / (
        "m23-4-resource-tests.exe"
        if (out / "m23-4-resource-tests.exe").exists()
        else "m23-4-resource-tests"
    )
    r = subprocess.run(
        [str(unit)], capture_output=True, text=True, check=False, cwd=str(ROOT)
    )
    assert r.returncode == 0, (r.stdout or "") + (r.stderr or "")

    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    multi = out / "app_multiwin.hxf"
    multi.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_multiwin.s")
    )
    ppm = out / "m23_4_multiwin.ppm"
    r2 = subprocess.run(
        [
            str(vm),
            str(runner),
            "--app",
            str(multi),
            "--gui",
            "--desktop",
            "--fb-dump",
            str(ppm),
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r2.stdout or "") + (r2.stderr or "")
    assert r2.returncode == 0 and "guest halted" in (r2.stdout or ""), blob
    rr, gg, bb = read_ppm_rgb(ppm, 40, 52)
    assert gg >= 200 and rr < 80 and bb < 80, (rr, gg, bb)

    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    ppm2 = out / "m23_4_desk.ppm"
    r3 = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            "m23-redraw",
            "--max-steps",
            "800000",
            "--fb-dump",
            str(ppm2),
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    blob3 = (r3.stdout or "") + (r3.stderr or "")
    assert r3.returncode == 0, blob3
    assert "guest halted" in (r3.stdout or ""), blob3
    assert ppm2.exists() and ppm2.stat().st_size > 100

    print("m23.4 composition tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
