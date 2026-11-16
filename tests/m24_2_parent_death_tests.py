#!/usr/bin/env python3
"""M24.2 E3: parent EXIT reparents children; init can WAIT orphans."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_lifecycle as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Middle EXIT without WAIT; init reaps middle and reparented grandchild."""
    if len(argv) != 2:
        print("usage: m24_2_parent_death_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_parent_death.hxf"
    img.write_bytes(b.build_parent_death())
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
    for ch in "MGPQZ":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m24.2 parent death tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
