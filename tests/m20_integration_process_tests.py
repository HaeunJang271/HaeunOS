#!/usr/bin/env python3
"""M20.8: process-level integration under AS slot limits (launcher + 2 apps)."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_launch  # noqa: E402


def run_vm(vm: Path, args: list[str], timeout: int = 180) -> tuple[int, str]:
    """Run haeun-vm and return (code, combined output)."""
    r = subprocess.run(
        [str(vm), *args],
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(ROOT),
    )
    return r.returncode, (r.stdout or "") + (r.stderr or "")


def main(argv: list[str]) -> int:
    """
    Concurrent capacity: HAEUN_APP_SLOTS=3 (launcher + 2 apps).
    Full simultaneous launcher+SM+filemgr+calc+edit+IPC+input+net is NOT
    possible; this run verifies launcher multi-app spawn + sequential IPC/file.
    """
    if len(argv) != 2:
        print(
            "usage: m20_integration_process_tests.py <haeun-vm>",
            file=sys.stderr,
        )
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    (out / "process_launch.hxf").write_bytes(build_process_launch.build())
    for name, src, extras in (
        ("app_calc.hxf", "app_calc.s", []),
        ("app_edit.hxf", "app_edit.s", []),
        ("app_files.hxf", "app_files.s", []),
        ("app_svc_idle.hxf", "app_svc_idle.s", []),
        ("app_filemgr.hxf", "app_filemgr.s", [ROOT / "userland" / "libapp.s"]),
    ):
        (out / name).write_bytes(
            build_app_hxf.build_app(ROOT / "userland" / src, extras)
        )

    code, blob = run_vm(vm, [str(out / "process_launch.hxf"), "--apps"])
    if code != 0:
        print(blob, file=sys.stderr)
        return 1
    try:
        assert "guest halted" in blob, blob
        assert "[haeun-fault]" not in blob, blob
        assert "app-env: spawn calc" in blob, blob
        assert "app-env: spawn edit" in blob, blob
        assert "app-env: spawn svcidle" in blob, blob
    except AssertionError as exc:
        print(exc, file=sys.stderr)
        print(blob, file=sys.stderr)
        return 1

    # Sequential filemgr under process_sdk (IPC dual covered by m20_ipc_process).
    import build_process_sdk  # noqa: E402

    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    code, blob = run_vm(
        vm,
        [
            str(runner),
            "--app",
            str(out / "app_filemgr.hxf"),
            "--gui",
            "--desktop",
        ],
    )
    if code != 0:
        print(blob, file=sys.stderr)
        return 1
    if "guest halted" not in blob or "[haeun-fault]" in blob:
        print(blob, file=sys.stderr)
        return 1
    if "app-env: spawn app_filemgr" not in blob:
        print(blob, file=sys.stderr)
        return 1

    print(
        "m20 integration process tests passed "
        "(concurrent AS slots=3; full multi-app simultaneous = NOT feasible)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
