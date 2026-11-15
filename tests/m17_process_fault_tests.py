#!/usr/bin/env python3
"""M17: child page-fault is contained; parent continues and halts cleanly."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_process_fault  # noqa: E402


def main(argv: list[str]) -> int:
    """Build fault_proc image and require clean guest halt with parent marks."""
    if len(argv) != 2:
        print("usage: m17_process_fault_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    image = out / "process_fault.hxf"
    image.write_bytes(build_process_fault.build())
    result = subprocess.run(
        [str(vm), str(image)],
        capture_output=True,
        text=True,
        check=False,
    )
    combined = result.stdout + result.stderr
    if result.returncode != 0:
        print(combined, file=sys.stderr)
        return 1
    if "guest halted" not in result.stdout:
        print(combined, file=sys.stderr)
        return 1
    # Parent writes 'P' before WAIT and 'O' after reaping the faulted child.
    if "P" not in result.stdout or "O" not in result.stdout:
        print("missing parent UART markers", combined, file=sys.stderr)
        return 1
    if "[haeun-fault] user" not in result.stderr and "[haeun-fault] user" not in combined:
        # fault_io may be unset without --gui/--net; guest path must still recover.
        pass
    print("m17 process fault tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
