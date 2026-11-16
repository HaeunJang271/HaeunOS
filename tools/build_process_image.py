#!/usr/bin/env python3
"""Shared linker for process-v1 kernel + a user program at 0x00108000."""
from __future__ import annotations

from pathlib import Path

import haeun_toolchain as tc

ROOT = Path(__file__).resolve().parents[1]
HXF_BASE = 0x00100000
USER_VA = 0x00108000
KERNEL_SRCS = [
    ROOT / "kernel/process/proc_boot.s",
    ROOT / "kernel/process/proc_trap.s",
    ROOT / "kernel/process/proc_sys.s",
    ROOT / "kernel/process/proc_fork.s",
    ROOT / "kernel/process/proc_fork_mem.s",
    ROOT / "kernel/process/proc_orphan.s",
    ROOT / "kernel/process/proc_sched.s",
]


def _symbol_va(objects: list, placements: dict, name: str) -> int:
    """Return linked VA of a defined global symbol."""
    for oi, obj in enumerate(objects):
        for sym in obj["symbols"]:
            if sym["defined"] and sym["name"] == name:
                off = placements[(oi, sym["section"])] + sym["value"]
                return HXF_BASE + off
    raise tc.ToolchainError(f"missing symbol {name}")


def _place(objects: list) -> tuple[dict, dict]:
    """Mimic link_hobjs section placement."""
    placements: dict = {}
    areas = {".text": bytearray(), ".data": bytearray()}
    for oi, obj in enumerate(objects):
        for si, sec in enumerate(obj["sections"]):
            area = areas[sec["name"]]
            start = tc.align_up(len(area), sec["alignment"])
            area.extend(bytes(start - len(area)))
            placements[(oi, si)] = start
            area.extend(sec["data"])
    return placements, areas


def _pad_bytes(n: int) -> bytes:
    """Assemble a padding object of exactly n zero bytes in .text."""
    if n <= 0 or n % 4:
        raise tc.ToolchainError("pad size must be positive and 4-aligned")
    lines = [".isa 4", ".global _proc_pad", "_proc_pad:"]
    lines.extend(["    .word 0"] * (n // 4))
    return tc.assemble("\n".join(lines) + "\n")


def build_with_user(
    user_source: Path, extra_sources: list[Path] | None = None
) -> bytes:
    """
    Link process kernel with user_source as user_init at 0x00108000.

    Optional extra_sources are linked after the primary user object (helpers).
    Patches trapvec to the linked trap_entry address.
    """
    kernel_objs = [
        tc.assemble(path.read_text(encoding="utf-8")) for path in KERNEL_SRCS
    ]
    placements, areas = _place([tc.parse_hobj(o) for o in kernel_objs])
    trap_va = _symbol_va(
        [tc.parse_hobj(o) for o in kernel_objs], placements, "trap_entry"
    )
    pad_size = USER_VA - HXF_BASE - len(areas[".text"])
    if pad_size < 0:
        raise tc.ToolchainError("kernel .text exceeds user base 0x00108000")
    user_obj = tc.assemble(user_source.read_text(encoding="utf-8"))
    blobs = list(kernel_objs)
    if pad_size:
        blobs.append(_pad_bytes(pad_size))
    blobs.append(user_obj)
    for extra in extra_sources or []:
        blobs.append(tc.assemble(extra.read_text(encoding="utf-8")))
    boot_text = KERNEL_SRCS[0].read_text(encoding="utf-8")
    old = "    jal r1, p_tv\n    .word 0x00100000\np_tv:"
    new = f"    jal r1, p_tv\n    .word 0x{trap_va:08X}\np_tv:"
    if old not in boot_text:
        raise tc.ToolchainError("proc_boot trapvec site not found for patch")
    blobs[0] = tc.assemble(boot_text.replace(old, new, 1))
    image = tc.link_hobjs(blobs)
    final_objs = [tc.parse_hobj(b) for b in blobs]
    pl, _ = _place(final_objs)
    user_va = _symbol_va(final_objs, pl, "user_init")
    if user_va != USER_VA:
        raise tc.ToolchainError(
            f"user_init at 0x{user_va:08x}, expected 0x{USER_VA:08x}"
        )
    return image
