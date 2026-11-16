#!/usr/bin/env python3
"""M24.2 E2: WAIT before/after child EXIT; SPAWN orphan PCB reclaim."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_lifecycle as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Guest WAIT pairing plus host SPAWN zombie reclaim stress."""
    if len(argv) != 2:
        print("usage: m24_2_orphan_wait_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_orphan_wait.hxf"
    img.write_bytes(b.build_orphan_wait())
    r = subprocess.run(
        [str(vm), str(img)],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0 and "guest halted" in r.stdout, blob
    for ch in "B12D":
        assert ch in blob, f"missing {ch}: {blob}"
    # WAIT takes no pid; unrelated/invalid selective WAIT is unsupported by ABI.
    print("m24.2 orphan wait tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
