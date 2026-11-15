#!/usr/bin/env python3
"""Link process-v1 kernel + SDK runner (SPAWN all --app registrations)."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_image as bpi  # noqa: E402


def build() -> bytes:
    """Return process_sdk HXF bytes."""
    return bpi.build_with_user(ROOT / "userland" / "sdk_runner.s")


def main(argv: list[str] | None = None) -> int:
    """Write process_sdk.hxf."""
    args = argv if argv is not None else sys.argv[1:]
    if len(args) != 1:
        print("usage: build_process_sdk.py <out.hxf>", file=sys.stderr)
        return 2
    out = Path(args[0])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build())
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
