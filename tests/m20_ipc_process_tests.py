#!/usr/bin/env python3
"""M20.2: concurrent IPC sender/receiver and pingpong under ipc_dual runner."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_image as bpi  # noqa: E402


def run_pair(vm: Path, a_src: str, b_src: str, an: str, bn: str) -> str:
    """Build dual runner + two apps; spawn concurrently."""
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    runner = out / "ipc_dual.hxf"
    runner.write_bytes(bpi.build_with_user(ROOT / "userland" / "ipc_dual.s"))
    ha = out / f"{an}.hxf"
    hb = out / f"{bn}.hxf"
    ha.write_bytes(build_app_hxf.build_app_with_libapp(ROOT / "userland" / a_src))
    hb.write_bytes(build_app_hxf.build_app_with_libapp(ROOT / "userland" / b_src))
    r = subprocess.run(
        [str(vm), str(runner), "--app", str(ha), "--app", str(hb), "--gui", "--desktop"],
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
    """Run M20.2 process-level IPC checks."""
    if len(argv) != 2:
        print("usage: m20_ipc_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    run_pair(vm, "app_ipc_rx.s", "app_ipc_tx.s", "ipc_receiver", "ipc_sender")
    run_pair(vm, "app_ipc_sv.s", "app_ipc_cl.s", "ipc_pong_sv", "ipc_pong_cl")
    print("m20 ipc process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
