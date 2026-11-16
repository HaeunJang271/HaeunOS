#!/usr/bin/env python3
"""M24.2 E1: guest lifecycle contract (READY/RUNNING/WAIT/ZOMBIE/FREE)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_lifecycle as b  # noqa: E402
import build_process_fault  # noqa: E402


def _run(vm: Path, image: Path) -> str:
    """Execute guest image; return combined output."""
    r = subprocess.run(
        [str(vm), str(image)],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in r.stdout, blob
    return blob


def main(argv: list[str]) -> int:
    """Assert dual-stage EXIT markers and fault containment."""
    if len(argv) != 2:
        print("usage: m24_2_lifecycle_contract_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)

    life = out / "m24_lifecycle.hxf"
    life.write_bytes(b.build_lifecycle())
    blob = _run(vm, life)
    for ch in "ACRFX":
        assert ch in blob, f"missing marker {ch}: {blob}"

    fault = out / "process_fault.hxf"
    fault.write_bytes(build_process_fault.build())
    fblob = _run(vm, fault)
    assert "P" in fblob and "O" in fblob, fblob

    print("m24.2 lifecycle contract tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
