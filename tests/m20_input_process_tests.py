#!/usr/bin/env python3
"""M20.1: input_echo / mouse_trk / input_iso under process_sdk + scripted input."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402


def run(vm: Path, name: str, src: str, with_input: bool) -> str:
    """Build with libapp, run via process_sdk --app [--gui --desktop --input]."""
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    hxf = out / f"{name}.hxf"
    hxf.write_bytes(build_app_hxf.build_app_with_libapp(ROOT / "userland" / src))
    ps = out / "process_sdk.hxf"
    ps.write_bytes(build_process_sdk.build())
    cmd = [str(vm), str(ps), "--app", str(hxf), "--gui", "--desktop"]
    if with_input:
        cmd.append("--input")
    r = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    return blob


def main(argv: list[str]) -> int:
    """Run M20.1 process-level input checks."""
    if len(argv) != 2:
        print("usage: m20_input_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    echo = run(vm, "app_input_echo", "app_input_echo.s", with_input=True)
    assert "input-host:" in echo, echo
    assert "keys=" in echo, echo
    mouse = run(vm, "app_mouse_trk", "app_mouse_trk.s", with_input=True)
    assert "mice=" in mouse, mouse
    iso = run(vm, "app_input_iso", "app_input_iso.s", with_input=False)
    assert "guest halted" in iso
    print("m20 input process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
