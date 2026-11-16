#!/usr/bin/env python3
"""M25.2: run host memory scrub binary."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main(argv: list[str]) -> int:
    """Execute m25-2-memory-scrub-tests host binary."""
    host = Path(argv[2]) if len(argv) > 2 else ROOT / "build" / (
        "m25-2-memory-scrub-tests.exe"
        if sys.platform.startswith("win")
        else "m25-2-memory-scrub-tests"
    )
    if not host.is_file():
        alt = ROOT / "build" / "m25-2-memory-scrub-tests"
        if alt.is_file():
            host = alt
    r = subprocess.run([str(host)], capture_output=True, text=True, check=False)
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "m25_2_memory_scrub_tests ok" in r.stdout, blob
    print("m25.2 memory scrub tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
