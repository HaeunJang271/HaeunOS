#!/usr/bin/env python3
"""M11: real-time Win32 viewer shows guest FB and exits cleanly on halt."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_gui_demo  # noqa: E402
import build_input_demo  # noqa: E402


def run(vm: Path, image: Path, args: list[str]) -> subprocess.CompletedProcess[str]:
    """Run haeun-vm with args; return completed process."""
    return subprocess.run(
        [str(vm), str(image), *args],
        capture_output=True,
        text=True,
        check=False,
    )


def main(argv: list[str]) -> int:
    """Exercise --viewer with gui_demo and input_demo (scripted)."""
    if len(argv) != 2:
        print("usage: m11_viewer_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    if sys.platform != "win32":
        print("m11 viewer tests skipped (Win32 backend only)")
        return 0
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    gui = out / "gui_demo.hxf"
    gui.write_bytes(build_gui_demo.build())
    r = run(vm, gui, ["--gui", "--user", "--viewer"])
    if r.returncode != 0:
        print(r.stdout, r.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in r.stdout, r.stdout
    frames_line = [ln for ln in r.stdout.splitlines() if ln.startswith("viewer:")][0]
    n = int(frames_line.split("frames=")[1].split()[0])
    assert n >= 1, frames_line

    inp = out / "input_demo.hxf"
    inp.write_bytes(build_input_demo.build())
    r2 = run(vm, inp, ["--gui", "--user", "--input", "--viewer"])
    if r2.returncode != 0:
        print(r2.stdout, r2.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in r2.stdout, r2.stdout
    assert "r1=0x00000001" in r2.stdout, r2.stdout
    assert "focus_id=1" in r2.stdout, r2.stdout
    print("m11 viewer tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
