#!/usr/bin/env python3
"""M24.3: init reaps adopted orphans; idle TRYWAIT does not block."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_3 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Run single/multi orphan + idle contract image."""
    if len(argv) != 2:
        print("usage: m24_3_init_reaper_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_3_init_reaper.hxf"
    img.write_bytes(b.build_init_reaper())
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
    for ch in "SABIZ":
        assert ch in blob, f"missing {ch}: {blob}"
    assert "\xee" not in blob.lower() or "EE" not in blob
    print("m24.3 init reaper tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
