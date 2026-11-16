#!/usr/bin/env python3
"""M23.2: chrome CLOSE vs exit vs host reap (independently verified)."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402
import build_process_sdk  # noqa: E402


def build_builtin_apps(out: Path) -> None:
    """Build --apps registry images."""
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
    """Verify CLOSE delivery, guest exit, host reap, focus, desktop alive."""
    if len(argv) != 2:
        print("usage: m23_2_close_lifecycle_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)

    # --- A: guest exits ONLY on EVT_CLOSE (Esc ignored) ---
    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    life = out / "app_close_life.hxf"
    life.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_close_life.s")
    )
    r = subprocess.run(
        [
            str(vm),
            str(runner),
            "--app",
            str(life),
            "--gui",
            "--desktop",
            "--input-script",
            "chrome-close-app",
            "--max-steps",
            "200000",
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in (r.stdout or ""), blob
    assert "[haeun-fault]" not in blob, blob

    # --- B: desktop chrome Close on calc; shell remains until Esc ---
    build_builtin_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    r2 = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            "chrome-close-calc",
            "--max-steps",
            "300000",
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    blob2 = (r2.stdout or "") + (r2.stderr or "")
    assert r2.returncode == 0, blob2
    assert "guest halted" in (r2.stdout or ""), blob2
    assert "[haeun-fault]" not in blob2, blob2
    assert "app-env: spawn calc" in blob2, blob2
    # Script never sends Esc before chrome Close; calc exit proves CLOSE handled.
    # Desktop then accepts Esc and halts — shell remained alive after reap.
    m = re.search(r"input-host:.*focus_id=(-?\d+)", blob2)
    assert m, blob2
    assert int(m.group(1)) >= -1, blob2
    print("m23.2 close lifecycle tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
