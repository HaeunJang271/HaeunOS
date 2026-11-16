#!/usr/bin/env python3
"""M24.3: repeated orphan lifecycle; no unreclaimed hang."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_3 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Four parent/child orphan cycles reaped by init WAIT."""
    if len(argv) != 2:
        print("usage: m24_3_orphan_stress_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_3_orphan_stress.hxf"
    img.write_bytes(b.build_orphan_stress())
    r = subprocess.run(
        [str(vm), str(img)],
        capture_output=True,
        text=True,
        check=False,
        timeout=120,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0 and "guest halted" in r.stdout, blob
    assert blob.count("R") >= 4, blob
    assert "T" in blob, blob
    print("m24.3 orphan stress tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
