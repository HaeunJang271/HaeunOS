#!/usr/bin/env python3
"""Assemble and link the guest desktop demo HXF."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402


def build() -> bytes:
    """Return a linked HXF image for userland/desktop_demo.s."""
    src = (ROOT / "userland" / "desktop_demo.s").read_text(encoding="utf-8")
    return tc.link_hobjs([tc.assemble(src)])


def main(argv: list[str] | None = None) -> int:
    """Write desktop_demo.hxf to the given path."""
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 1:
        print("usage: build_desktop_demo.py <out.hxf>", file=sys.stderr)
        return 2
    out = Path(args[0])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build())
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
