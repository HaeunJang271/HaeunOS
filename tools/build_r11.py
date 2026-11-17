#!/usr/bin/env python3
"""Build R1.1 guest images and print linked process-v1 scheduler symbols."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402
import build_process_image as bpi  # noqa: E402

NAMES = (
    "r11_yield",
    "r11_sleep",
    "r11_sign",
    "r11_wait",
    "r11_sibling",
    "r11_fault",
    "r11_dead",
    "r11_spin",
)


def main(argv: list[str]) -> int:
    """Assemble each R1.1 user program onto the process-v1 kernel."""
    out = Path(argv[1] if len(argv) > 1 else ROOT / "build")
    out.mkdir(parents=True, exist_ok=True)
    objs = [tc.assemble(path.read_text(encoding="utf-8")) for path in bpi.KERNEL_SRCS]
    parsed = [tc.parse_hobj(blob) for blob in objs]
    placements, _areas = bpi._place(parsed)
    for name in ("trap_entry", "idle_enter", "idle_trap", "sched_slots", "arm_timer"):
        va = bpi._symbol_va(parsed, placements, name)
        print(f"linked {name} 0x{va:08x}")
    for name in NAMES:
        image = bpi.build_with_user(ROOT / "userland" / f"{name}.s")
        dest = out / f"{name}.hxf"
        dest.write_bytes(image)
        print(f"wrote {dest.name} {len(image)} bytes")
    (out / "r11-images.stamp").write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
