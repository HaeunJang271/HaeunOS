#!/usr/bin/env python3
"""M20.7: file manager app under process_sdk."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402


def main(argv: list[str]) -> int:
    """Build filemgr with libapp and run via process_sdk --app --apps HFS."""
    if len(argv) != 2:
        print("usage: m20_file_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    hxf = out / "app_filemgr.hxf"
    hxf.write_bytes(
        build_app_hxf.build_app(
            ROOT / "userland" / "app_filemgr.s",
            [ROOT / "userland" / "libapp.s"],
        )
    )
    r = subprocess.run(
        [str(vm), str(runner), "--app", str(hxf), "--gui", "--desktop"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    if r.returncode != 0:
        print(blob, file=sys.stderr)
        return 1
    assert "guest halted" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    assert "app-env: spawn app_filemgr" in r.stdout, blob
    print("m20 file process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
