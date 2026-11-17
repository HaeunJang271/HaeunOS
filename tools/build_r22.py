#!/usr/bin/env python3
"""Build R2.2 evaluation guest images (reuse R1.2 + r22_burst)."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_image as bpi  # noqa: E402
import build_r12  # noqa: E402

NAMES = ("r22_burst",)


def main(argv: list[str]) -> int:
    """Assemble R2.2 workloads and ensure R1.2 images exist."""
    out = Path(argv[1] if len(argv) > 1 else ROOT / "build")
    out.mkdir(parents=True, exist_ok=True)
    rc = build_r12.main([argv[0], str(out)])
    if rc != 0:
        return rc
    for name in NAMES:
        image = bpi.build_with_user(ROOT / "userland" / f"{name}.s")
        dest = out / f"{name}.hxf"
        dest.write_bytes(image)
        print(f"wrote {dest.name} {len(image)} bytes")
    (out / "r22-images.stamp").write_text("ok\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
