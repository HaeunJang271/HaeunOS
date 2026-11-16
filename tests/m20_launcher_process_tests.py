#!/usr/bin/env python3
"""M20.5: GUI launcher process image under --apps."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_launch  # noqa: E402


def main(argv: list[str]) -> int:
    """Build process_launch + builtin apps; require spawn markers and clean halt."""
    if len(argv) != 2:
        print("usage: m20_launcher_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    (out / "process_launch.hxf").write_bytes(build_process_launch.build())
    for name, src in (
        ("app_calc.hxf", "app_calc.s"),
        ("app_edit.hxf", "app_edit.s"),
        ("app_files.hxf", "app_files.s"),
        ("app_svc_idle.hxf", "app_svc_idle.s"),
        ("app_filemgr.hxf", "app_filemgr.s"),
    ):
        extras = []
        if src == "app_filemgr.s":
            extras = [ROOT / "userland" / "libapp.s"]
        (out / name).write_bytes(
            build_app_hxf.build_app(ROOT / "userland" / src, extras)
        )

    result = subprocess.run(
        [str(vm), str(out / "process_launch.hxf"), "--apps"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (result.stdout or "") + (result.stderr or "")
    if result.returncode != 0:
        print(blob, file=sys.stderr)
        return 1
    try:
        assert "guest halted" in result.stdout, result.stdout
        assert "[haeun-fault]" not in blob, blob
        assert "apps: registered calc" in result.stdout, blob
        assert "app-env: spawn calc" in result.stdout, blob
        assert "app-env: spawn edit" in result.stdout, blob
        assert "app-env: spawn NOSPC" in blob, blob
        assert "app-env: spawn filemgr" in result.stdout or (
            "app-env: spawn app_filemgr" in result.stdout
        ), blob
        assert "apps: registered svcidle" in result.stdout, blob
        assert "app-env: spawn svcidle" in result.stdout, blob
        assert "r1=0x00000001" in result.stdout, blob
    except AssertionError as exc:
        print(exc, file=sys.stderr)
        print(blob, file=sys.stderr)
        return 1
    print("m20 launcher process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
