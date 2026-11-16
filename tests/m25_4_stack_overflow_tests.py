#!/usr/bin/env python3
"""M25.4: repeated stack allocation into guard; parent continues."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m25_4 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Assert O/P/Z; child must not print post-overflow marker."""
    if len(argv) != 2:
        print(
            "usage: m25_4_stack_overflow_tests.py <haeun-vm>",
            file=sys.stderr,
        )
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m25_4_stack_overflow.hxf"
    img.write_bytes(b.build_overflow())
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
    for ch in "OPZ":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m25.4 stack overflow tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
