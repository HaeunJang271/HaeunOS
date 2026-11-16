#!/usr/bin/env python3
"""M25.3: FORK clones ustack + code page into child (snapshot markers)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m25_3 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Assert S/C/P/Z markers from snapshot guest."""
    if len(argv) != 2:
        print(
            "usage: m25_3_fork_memory_snapshot_tests.py <haeun-vm>",
            file=sys.stderr,
        )
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m25_3_fork_snapshot.hxf"
    img.write_bytes(b.build_snapshot())
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
    for ch in "SCPZ":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m25.3 fork memory snapshot tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
