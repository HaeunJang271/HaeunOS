#!/usr/bin/env python3
"""M9: net_proc through process-v1 with host-assisted Socket ECALLs."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_net  # noqa: E402


def main(argv: list[str]) -> int:
    """Build process_net.hxf, run with --net, check TX/RX and halt pid."""
    if len(argv) != 2:
        print("usage: m9_process_net_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "process_net.hxf"
    image.write_bytes(build_process_net.build())
    result = subprocess.run(
        [str(vm), str(image), "--net"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(result.stdout, result.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in result.stdout, result.stdout
    assert "r1=0x00000001" in result.stdout, result.stdout
    assert "TX frames=" in result.stdout, result.stdout
    assert "injected UDP reply" in result.stdout, result.stdout
    probe = out / ("m9-net-probe.exe" if sys.platform == "win32" else "m9-net-probe")
    if not probe.exists():
        print("missing m9-net-probe; build via make", file=sys.stderr)
        return 1
    probe_run = subprocess.run(
        [str(probe), str(image)], capture_output=True, text=True, check=False
    )
    if probe_run.returncode != 0:
        print(probe_run.stdout, probe_run.stderr, file=sys.stderr)
        return 1
    assert "net_ok" in probe_run.stdout
    print("m9 process net tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
