#!/usr/bin/env python3
"""M23.2: guest multi-window + capacity stress via real WIN_* ECALLs."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
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


def run_guest(vm: Path, runner: Path, app: Path, ppm: Path | None) -> str:
    """Run process_sdk with one guest app on the real WM path."""
    cmd = [str(vm), str(runner), "--app", str(app), "--gui", "--desktop"]
    if ppm is not None:
        cmd.extend(["--fb-dump", str(ppm)])
    r = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in (r.stdout or ""), blob
    assert "[haeun-fault]" not in blob, blob
    return blob


def main(argv: list[str]) -> int:
    """Build and run multi-window + capacity guests."""
    if len(argv) != 2:
        print("usage: m23_2_multi_window_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())

    multi = out / "app_multiwin.hxf"
    multi.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_multiwin.s")
    )
    ppm = out / "m23_2_multiwin.ppm"
    run_guest(vm, runner, multi, ppm)
    # Overlap sample after raise B (green on top): around (40, 52).
    r, g, b = read_ppm_rgb(ppm, 40, 52)
    assert g >= 200 and r < 80 and b < 80, (r, g, b, ppm)

    cap = out / "app_win_cap.hxf"
    cap.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_win_cap.s")
    )
    run_guest(vm, runner, cap, None)

    print("m23.2 multi-window tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
