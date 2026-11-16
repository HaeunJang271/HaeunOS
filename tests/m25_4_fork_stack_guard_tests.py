#!/usr/bin/env python3
"""M25.4: FORK child guard fault; parent intact; reused child gets clean stack."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m25_4 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Assert A/C/P/R/Z markers."""
    if len(argv) != 2:
        print(
            "usage: m25_4_fork_stack_guard_tests.py <haeun-vm>",
            file=sys.stderr,
        )
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m25_4_fork_stack_guard.hxf"
    img.write_bytes(b.build_fork_guard())
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
    for ch in "ACPRZ":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m25.4 fork stack guard tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
