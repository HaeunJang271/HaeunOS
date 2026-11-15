#!/usr/bin/env python3
"""Build process-v1 image whose child triggers a user page fault."""
from __future__ import annotations

from pathlib import Path

import build_process_image as bpi

ROOT = Path(__file__).resolve().parents[1]


def build() -> bytes:
    """Return linked HXF for fault_proc userland."""
    return bpi.build_with_user(ROOT / "userland" / "fault_proc.s")


def main() -> int:
    """Write process_fault.hxf under build/."""
    out = ROOT / "build" / "process_fault.hxf"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build())
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
