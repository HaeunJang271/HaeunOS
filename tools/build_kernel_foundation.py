#!/usr/bin/env python3
"""Assemble/compile/link the HAEUN-OS kernel foundation HXF image."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402

ASM = [
    ROOT / "kernel/arch/entry.s",
    ROOT / "kernel/arch/cpu.s",
    ROOT / "kernel/interrupt/trap.s",
    ROOT / "kernel/drivers/uart.s",
    ROOT / "kernel/mm/mem.s",
    ROOT / "kernel/core/klog_msg.s",
    ROOT / "kernel/core/kpanic_halt.s",
]
C_SOURCES = [
    ROOT / "kernel/core/klog.c",
    ROOT / "kernel/core/kpanic.c",
    ROOT / "kernel/core/kassert.c",
    ROOT / "kernel/core/kmain.c",
    ROOT / "kernel/mm/pmm.c",
    ROOT / "kernel/mm/vmm.c",
    ROOT / "kernel/mm/heap.c",
]


def build() -> bytes:
    """Return a linked foundation HXF (ISA 4, single .text segment)."""
    objects = [tc.assemble(path.read_text(encoding="utf-8")) for path in ASM]
    for path in C_SOURCES:
        objects.append(tc.compile_c(path.read_text(encoding="utf-8"), isa_version=4))
    return tc.link_hobjs(objects)


def main(argv: list[str] | None = None) -> int:
    """Write the foundation HXF to the given output path."""
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 1:
        print("usage: build_kernel_foundation.py <out.hxf>", file=sys.stderr)
        return 2
    out = Path(args[0])
    image = build()
    out.write_bytes(image)
    print(f"wrote {out} ({len(image)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
