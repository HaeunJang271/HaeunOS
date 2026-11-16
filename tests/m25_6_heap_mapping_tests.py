#!/usr/bin/env python3
"""M25.6: heap VA mapping guest + host PA/scrub checks."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m25_6 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Assert M/Z markers and optional host binary."""
    if len(argv) not in (2, 3):
        print(
            "usage: m25_6_heap_mapping_tests.py <haeun-vm> [host-bin]",
            file=sys.stderr,
        )
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m25_6_heap_mapping.hxf"
    img.write_bytes(b.build_mapping())
    r = subprocess.run(
        [str(vm), str(img)],
        capture_output=True,
        text=True,
        check=False,
        timeout=90,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0 and "guest halted" in r.stdout, blob
    for ch in "MZ":
        assert ch in blob, f"missing {ch}: {blob}"
    if len(argv) == 3:
        host = Path(argv[2])
        hr = subprocess.run(
            [str(host)], capture_output=True, text=True, check=False
        )
        assert hr.returncode == 0, hr.stdout + hr.stderr
    print("m25.6 heap mapping tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
