#!/usr/bin/env python3
"""M24.4 C: guest PID/slot reuse + foreign ZOMBIE not stolen by init TRYWAIT."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_m24_4 as b  # noqa: E402


def main(argv: list[str]) -> int:
    """Run pid-reuse guest image; markers ABXZ."""
    if len(argv) != 2:
        print("usage: m24_4_pid_reuse_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    img = out / "m24_4_pid_reuse.hxf"
    img.write_bytes(b.build_pid_reuse())
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
    for ch in "ABXZ":
        assert ch in blob, f"missing {ch}: {blob}"
    print("m24.4 pid reuse tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
