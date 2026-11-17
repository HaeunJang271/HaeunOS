#!/usr/bin/env python3
"""Build R1.2 telemetry guest images on the process-v1 kernel."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_image as bpi  # noqa: E402

NAMES = (
    "r12_run",
    "r12_yield",
    "r12_sleep",
    "r12_wait",
    "r12_pair",
    "r12_fault",
)


def main(argv: list[str]) -> int:
    """Assemble each R1.2 user program onto the process-v1 kernel."""
    out = Path(argv[1] if len(argv) > 1 else ROOT / "build")
    out.mkdir(parents=True, exist_ok=True)
    for name in NAMES:
        image = bpi.build_with_user(ROOT / "userland" / f"{name}.s")
        dest = out / f"{name}.hxf"
        dest.write_bytes(image)
        print(f"wrote {dest.name} {len(image)} bytes")
    (out / "r12-images.stamp").write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
