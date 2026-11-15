#!/usr/bin/env python3
"""M12: desktop_demo under --desktop --viewer; chrome unit binary separate."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_desktop_demo  # noqa: E402


def main(argv: list[str]) -> int:
    """Run desktop demo with viewer+desktop; require halt and frames."""
    if len(argv) != 2:
        print("usage: m12_desktop_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "desktop_demo.hxf"
    image.write_bytes(build_desktop_demo.build())
    if sys.platform == "win32":
        result = subprocess.run(
            [str(vm), str(image), "--desktop", "--user", "--viewer"],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            print(result.stdout, result.stderr, file=sys.stderr)
            return 1
        assert "guest halted" in result.stdout, result.stdout
        assert "r1=0x00000001" in result.stdout, result.stdout
        assert "viewer: frames=" in result.stdout, result.stdout
    else:
        result = subprocess.run(
            [str(vm), str(image), "--desktop", "--user", "--fb-dump",
             str(out / "desktop_demo.ppm")],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            print(result.stdout, result.stderr, file=sys.stderr)
            return 1
        assert "guest halted" in result.stdout, result.stdout
    print("m12 desktop tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
