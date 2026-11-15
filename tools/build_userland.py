#!/usr/bin/env python3
"""Link the HAEUN init+shell userland HXF image."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402

SOURCES = [
    ROOT / "userland/init.s",
    ROOT / "userland/libc.s",
    ROOT / "userland/shell.s",
    ROOT / "userland/cmds_info.s",
    ROOT / "kernel/vfs/block_drv.s",
    ROOT / "kernel/vfs/vfs_hfs.s",
]


def build() -> bytes:
    """Assemble and link the userland shell image."""
    objects = [tc.assemble(path.read_text(encoding="utf-8")) for path in SOURCES]
    return tc.link_hobjs(objects)


def main(argv: list[str] | None = None) -> int:
    """Write shell.hxf to the given path."""
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 1:
        print("usage: build_userland.py <out.hxf>", file=sys.stderr)
        return 2
    out = Path(args[0])
    out.write_bytes(build())
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
