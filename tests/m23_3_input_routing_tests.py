#!/usr/bin/env python3
"""M23.3: keyboard routing between two concurrent guest apps (real evt path)."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402

SC_A = "0x1e"
SC_B = "0x30"


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


def spawn_pid(blob: str, name: str) -> int:
    """Parse app-env spawn pid for a builtin app name."""
    m = re.search(rf"app-env: spawn {name} pid=(\d+)", blob)
    assert m, (name, blob)
    return int(m.group(1))


def key_downs(blob: str, sc: str) -> list[int]:
    """Return owner pids for KEY_DOWN lines of scancode sc."""
    return [
        int(m.group(1))
        for m in re.finditer(
            rf"\[haeun-evt\] key pid=(\d+) sc={sc} down", blob, re.I
        )
    ]


def main(argv: list[str]) -> int:
    """Verify keys follow focused app; minimize removes calc from eligibility."""
    if len(argv) != 2:
        print("usage: m23_3_input_routing_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    r = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            "m23-input-route",
            "--max-steps",
            "800000",
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

    calc = spawn_pid(blob, "calc")
    edit = spawn_pid(blob, "edit")
    assert calc != edit

    a_pids = key_downs(blob, SC_A)
    b_pids = key_downs(blob, SC_B)
    # Script: A(calc), B(edit), A(calc), B(edit after min recover)
    assert a_pids == [calc, calc], (a_pids, calc, edit, blob)
    assert b_pids == [edit, edit], (b_pids, calc, edit, blob)
    # No cross-leak of A to edit or B to calc.
    assert edit not in a_pids, blob
    assert calc not in b_pids, blob

    print("m23.3 input routing tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
