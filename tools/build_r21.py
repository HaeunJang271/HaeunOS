#!/usr/bin/env python3
"""Emit process-v1 policy symbol VAs for R2.1 guest policy call tests."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402
import build_process_image as bpi  # noqa: E402


def main(argv: list[str]) -> int:
    """Write build/r21-symbols.txt with linked policy_select VA."""
    out = Path(argv[1] if len(argv) > 1 else ROOT / "build")
    out.mkdir(parents=True, exist_ok=True)
    objs = [tc.assemble(path.read_text(encoding="utf-8")) for path in bpi.KERNEL_SRCS]
    parsed = [tc.parse_hobj(blob) for blob in objs]
    placements, _ = bpi._place(parsed)
    lines = []
    for name in ("policy_select", "policy_rr", "policy_fcfs", "pick_next",
                 "sched_slots", "sched_pol_boot"):
        va = bpi._symbol_va(parsed, placements, name)
        lines.append(f"{name} 0x{va:08x}")
        print(f"linked {name} 0x{va:08x}")
    (out / "r21-symbols.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (out / "r21-images.stamp").write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
