#!/usr/bin/env python3
"""M20: text / IPC / files apps under process_sdk; reject faults."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402


def run_app(vm: Path, name: str, src: str) -> str:
    """Build with libapp, run via process_sdk --app, return stdout+stderr."""
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    hxf = out / f"{name}.hxf"
    hxf.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / src)
    )
    ps = out / "process_sdk.hxf"
    ps.write_bytes(build_process_sdk.build())
    r = subprocess.run(
        [str(vm), str(ps), "--app", str(hxf)],
        capture_output=True,
        text=True,
        check=False,
        timeout=120,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    assert f"apps: registered {hxf.stem}" in r.stdout or "apps: registered" in r.stdout
    return blob


def main(argv: list[str]) -> int:
    """Run M20 process-level checks."""
    if len(argv) != 2:
        print("usage: m20_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    run_app(vm, "app_m20_text", "app_m20_text.s")
    run_app(vm, "app_m20_ipc", "app_m20_ipc.s")
    run_app(vm, "app_m20_files", "app_m20_files.s")
    # Calculator example must also be fault-free after reloc fix.
    calc = ROOT / "sdk" / "examples" / "calculator"
    subprocess.check_call(
        [sys.executable, str(ROOT / "sdk" / "haeun.py"), "build", str(calc)],
        cwd=str(ROOT),
    )
    out = ROOT / "build"
    ps = out / "process_sdk.hxf"
    ps.write_bytes(build_process_sdk.build())
    r = subprocess.run(
        [str(vm), str(ps), "--app", str(calc / "build" / "app.hxf")],
        capture_output=True,
        text=True,
        timeout=120,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0 and "guest halted" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    print("m20 process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
