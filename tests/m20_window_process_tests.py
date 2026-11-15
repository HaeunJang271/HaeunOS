#!/usr/bin/env python3
"""M20.4: window event demo apps under process_sdk."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_sdk  # noqa: E402

APPS = (
    ("app_win_ev", "app_win_ev.s"),
    ("app_win_rsz", "app_win_rsz.s"),
    ("app_win_cls", "app_win_cls.s"),
)


def main(argv: list[str]) -> int:
    """Build and run M20.4 window event apps."""
    if len(argv) != 2:
        print("usage: m20_window_process_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    for name, src in APPS:
        hxf = out / f"{name}.hxf"
        hxf.write_bytes(
            build_app_hxf.build_app_with_libapp(ROOT / "userland" / src)
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
        assert r.returncode == 0, (name, blob)
        assert "guest halted" in r.stdout, (name, blob)
        assert "[haeun-fault]" not in blob, (name, blob)
    print("m20 window process tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
