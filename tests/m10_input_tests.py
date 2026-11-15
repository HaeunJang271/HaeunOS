#!/usr/bin/env python3
"""M10: guest input_demo with scripted host keyboard/mouse injection."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_input_demo  # noqa: E402


def main(argv: list[str]) -> int:
    """Build input_demo.hxf, run --gui --user --input, check halt and focus."""
    if len(argv) != 2:
        print("usage: m10_input_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "input_demo.hxf"
    image.write_bytes(build_input_demo.build())
    result = subprocess.run(
        [str(vm), str(image), "--gui", "--user", "--input"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        print(result.stdout, result.stderr, file=sys.stderr)
        return 1
    assert "guest halted" in result.stdout, result.stdout
    assert "r1=0x00000001" in result.stdout, result.stdout
    assert "input-host: keys=2 mice=1 focus_hits=1" in result.stdout, result.stdout
    assert "focus_id=1" in result.stdout, result.stdout
    print("m10 input tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
