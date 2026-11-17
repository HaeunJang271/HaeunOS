#!/usr/bin/env python3
"""Build R3.0 RR (default) and FCFS evaluation guest images."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_image as bpi  # noqa: E402
import build_r12  # noqa: E402

NAMES = (
    "r12_run",
    "r12_yield",
    "r12_sleep",
    "r12_wait",
    "r12_pair",
    "r22_burst",
)


def main(argv: list[str]) -> int:
    """Write default RR images plus build/fcfs/* with policy_id=1."""
    out = Path(argv[1] if len(argv) > 1 else ROOT / "build")
    out.mkdir(parents=True, exist_ok=True)
    fcfs = out / "fcfs"
    fcfs.mkdir(parents=True, exist_ok=True)
    rc = build_r12.main([argv[0], str(out)])
    if rc != 0:
        return rc
    # Ensure burst image for RR eval path.
    burst = bpi.build_with_user(ROOT / "userland" / "r22_burst.s", policy_id=0)
    (out / "r22_burst.hxf").write_bytes(burst)
    print(f"wrote r22_burst.hxf {len(burst)} bytes")
    for name in NAMES:
        src = ROOT / "userland" / f"{name}.s"
        image = bpi.build_with_user(src, policy_id=1)
        dest = fcfs / f"{name}.hxf"
        dest.write_bytes(image)
        print(f"wrote fcfs/{dest.name} {len(image)} bytes")
    (out / "r30-images.stamp").write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
