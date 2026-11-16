#!/usr/bin/env python3
"""M24.4 A/B/D: WAIT/TRYWAIT interaction, fault EXIT, orphan cycles."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_4 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Run integrated lifecycle guest image."""
    if len(argv) != 2:
        print("usage: m24_4_lifecycle_integration_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_4_lifecycle.hxf"
    img.write_bytes(b.build_lifecycle())
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
    for ch in "STWNFZ":
        assert ch in blob, f"missing {ch}: {blob}"
    assert blob.count("R") >= 3, blob
    print("m24.4 lifecycle integration tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
