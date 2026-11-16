#!/usr/bin/env python3
"""M24.4 E: host resource reclaim-once and PID reuse without stale refs."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main(argv: list[str]) -> int:
    """Execute host C reclamation binary (built by make test)."""
    if len(argv) < 2:
        print(
            "usage: m24_4_resource_reclamation_tests.py <haeun-vm> [host-bin]",
            file=sys.stderr,
        )
        return 2
    host = Path(argv[2]) if len(argv) > 2 else ROOT / "build" / (
        "m24-4-resource-reclamation-tests.exe"
        if sys.platform.startswith("win")
        else "m24-4-resource-reclamation-tests"
    )
    if not host.is_file():
        alt = ROOT / "build" / "m24-4-resource-reclamation-tests"
        host = alt if alt.is_file() else host
    r = subprocess.run([str(host)], capture_output=True, text=True, check=False)
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "m24_4_resource_reclamation_tests ok" in r.stdout, blob
    print("m24.4 resource reclamation tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
