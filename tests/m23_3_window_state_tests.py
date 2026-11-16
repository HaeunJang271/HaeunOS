#!/usr/bin/env python3
"""M23.3: run host window-state unit binary + guest capacity stress."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402


def run(cmd: list[str], timeout: int = 180) -> str:
    """Run a command; return combined output; require exit 0."""
    r = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    return blob


def main(argv: list[str]) -> int:
    """Execute M23.3 window-state checks."""
    if len(argv) != 2:
        print("usage: m23_3_window_state_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    unit = out / (
        "m23-3-window-state-tests.exe"
        if (out / "m23-3-window-state-tests.exe").exists()
        else "m23-3-window-state-tests"
    )
    blob = run([str(unit)])
    assert "m23.3 window state tests passed" in blob, blob

    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    cap = out / "app_win_cap.hxf"
    cap.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_win_cap.s")
    )
    blob2 = run([str(vm), str(runner), "--app", str(cap), "--gui", "--desktop"])
    assert "guest halted" in blob2, blob2
    assert "[haeun-fault]" not in blob2, blob2

    multi = out / "app_multiwin.hxf"
    multi.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_multiwin.s")
    )
    blob3 = run([str(vm), str(runner), "--app", str(multi), "--gui", "--desktop"])
    assert "guest halted" in blob3, blob3
    assert "[haeun-fault]" not in blob3, blob3

    print("m23.3 window state tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
