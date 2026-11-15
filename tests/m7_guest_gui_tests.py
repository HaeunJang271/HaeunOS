#!/usr/bin/env python3
"""Build and run the guest gui_demo through haeun-vm; verify PPM RGB bars."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_gui_demo  # noqa: E402


def read_ppm_pixel(path: Path, x: int, y: int, width: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from a binary P6 PPM."""
    data = path.read_bytes()
    if not data.startswith(b"P6\n"):
        raise AssertionError("missing P6 magic")
    parts = data.split(b"\n", 3)
    dims = parts[1].decode().split()
    assert dims == [str(width), "32"], dims
    assert parts[2] == b"255"
    body = parts[3]
    off = (y * width + x) * 3
    return body[off], body[off + 1], body[off + 2]


def main(argv: list[str]) -> int:
    """Assemble gui_demo, run haeun-vm --gui --fb-dump, check pixels."""
    if len(argv) != 2:
        print("usage: m7_guest_gui_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out_dir = ROOT / "build"
    out_dir.mkdir(parents=True, exist_ok=True)
    image = out_dir / "gui_demo.hxf"
    ppm = out_dir / "gui_demo.ppm"
    image.write_bytes(build_gui_demo.build())
    result = subprocess.run(
        [str(vm), str(image), "--gui", "--user", "--fb-dump", str(ppm)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(result.stdout, result.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in result.stdout
    assert ppm.is_file()
    assert read_ppm_pixel(ppm, 0, 0, 64) == (255, 0, 0)
    assert read_ppm_pixel(ppm, 16, 0, 64) == (0, 255, 0)
    assert read_ppm_pixel(ppm, 32, 0, 64) == (0, 0, 255)
    assert read_ppm_pixel(ppm, 48, 0, 64) == (0, 0, 0)
    print("m7 guest gui tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
