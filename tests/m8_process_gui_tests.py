#!/usr/bin/env python3
"""M8: gui_proc through process-v1 PCB/scheduler with host-assisted GUI ECALLs."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_gui  # noqa: E402

def read_ppm_pixel(path: Path, x: int, y: int, width: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from a binary P6 PPM."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6"
    assert parts[1].decode().split() == [str(width), "32"]
    body = parts[3]
    off = (y * width + x) * 3
    return body[off], body[off + 1], body[off + 2]


def main(argv: list[str]) -> int:
    """Build process+GUI image, run with --gui, verify PCB and PPM."""
    if len(argv) != 2:
        print("usage: m8_process_gui_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "process_gui.hxf"
    ppm = out / "process_gui.ppm"
    image.write_bytes(build_process_gui.build())

    # Capture guest RAM via a second harness: run VM then read PCB from a
    # side channel — haeun-vm does not dump RAM, so embed checks in stdout
    # and PPM; PCB fields are verified by a tiny companion C probe built
    # into this test via subprocess on m8_pcb_probe if present.
    result = subprocess.run(
        [str(vm), str(image), "--gui", "--fb-dump", str(ppm)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(result.stdout, result.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in result.stdout, result.stdout
    # EXIT(code=pid) → r1 holds exit code when last process halts.
    assert "r1=0x00000001" in result.stdout, result.stdout
    assert ppm.is_file()
    assert read_ppm_pixel(ppm, 0, 0, 64) == (255, 0, 0)
    assert read_ppm_pixel(ppm, 16, 0, 64) == (0, 255, 0)
    assert read_ppm_pixel(ppm, 32, 0, 64) == (0, 0, 255)
    assert read_ppm_pixel(ppm, 48, 0, 64) == (0, 0, 0)

    # Host-side PCB inspection through a dedicated probe binary.
    probe = out / ("m8-pcb-probe.exe" if sys.platform == "win32" else "m8-pcb-probe")
    if not probe.exists():
        print("missing m8-pcb-probe; build it via make", file=sys.stderr)
        return 1
    probe_run = subprocess.run(
        [str(probe), str(image)],
        capture_output=True,
        text=True,
        check=False,
    )
    if probe_run.returncode != 0:
        print(probe_run.stdout, probe_run.stderr, file=sys.stderr)
        return 1
    assert "pcb_ok" in probe_run.stdout
    print("m8 process gui tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
