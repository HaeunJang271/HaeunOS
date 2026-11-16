#!/usr/bin/env python3
"""M23.4: integrated desktop scenarios A/B (real WM + guest evt path)."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402

SC_A, SC_B = "0x1e", "0x30"


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


def run_desk(vm: Path, desk: Path, script: str, steps: int) -> str:
    """Run persist desktop with named input script."""
    r = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            script,
            "--max-steps",
            str(steps),
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
    return blob


def spawn_pid(blob: str, name: str) -> int:
    """Parse first spawn pid for app name."""
    m = re.search(rf"app-env: spawn {name} pid=(\d+)", blob)
    assert m, (name, blob)
    return int(m.group(1))


def key_downs(blob: str, sc: str) -> list[int]:
    """Owner pids for KEY_DOWN of sc."""
    return [
        int(m.group(1))
        for m in re.finditer(
            rf"\[haeun-evt\] key pid=(\d+) sc={sc} down", blob, re.I
        )
    ]


def main(argv: list[str]) -> int:
    """Scenario A (desktop slice) + Scenario B chrome CLOSE."""
    if len(argv) != 2:
        print("usage: m23_4_integration_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())

    # Host Scenario A (third window, restore, destroy, compose counters).
    unit = out / (
        "m23-4-resource-tests.exe"
        if (out / "m23-4-resource-tests.exe").exists()
        else "m23-4-resource-tests"
    )
    r = subprocess.run(
        [str(unit)], capture_output=True, text=True, check=False, cwd=str(ROOT)
    )
    blob_u = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob_u
    assert "m23.4-A:" in blob_u, blob_u

    # Desktop Scenario A: raise/keys/min/recover (+ host covers restore).
    blob = run_desk(vm, desk, "m23-stab-a", 900000)
    calc, edit = spawn_pid(blob, "calc"), spawn_pid(blob, "edit")
    assert calc != edit
    a_pids, b_pids = key_downs(blob, SC_A), key_downs(blob, SC_B)
    assert all(p == calc for p in a_pids), (a_pids, calc, blob)
    assert all(p == edit for p in b_pids), (b_pids, edit, blob)
    assert len(a_pids) >= 2 and len(b_pids) >= 2, (a_pids, b_pids)

    # Scenario B: chrome CLOSE → calc exit; desktop Esc halt.
    blob_b = run_desk(vm, desk, "m23-stab-b", 600000)
    assert "app-env: spawn calc" in blob_b, blob_b
    assert re.search(r"input-host:.*focus_id=(-?\d+)", blob_b), blob_b

    print("m23.4 integration tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
