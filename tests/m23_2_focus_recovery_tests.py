#!/usr/bin/env python3
"""M23.2: focus recovery after minimize/destroy (host unit + guest desktop)."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402


def build_apps(out: Path) -> None:
    """Build builtin --apps images."""
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


def main(argv: list[str]) -> int:
    """Run focus-recovery desktop script and require clean halt."""
    if len(argv) != 2:
        print("usage: m23_2_focus_recovery_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    ppm = out / "m23_2_focus.ppm"
    r = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            "m23-focus-min",
            "--max-steps",
            "400000",
            "--fb-dump",
            str(ppm),
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in (r.stdout or ""), blob
    assert "[haeun-fault]" not in blob, blob
    assert "app-env: spawn calc" in blob, blob
    assert "app-env: spawn edit" in blob, blob
    # End-of-run focus after Esc may be -1 (documented no-focus); must not fault.
    m = re.search(r"input-host:.*focus_hits=(\d+) focus_id=(-?\d+)", blob)
    assert m, blob
    assert int(m.group(1)) >= 1, blob
    assert int(m.group(2)) >= -1, blob
    print("m23.2 focus recovery tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
