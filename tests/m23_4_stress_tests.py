#!/usr/bin/env python3
"""M23.4: focus/input stress + capacity guest (Scenario C path)."""
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


def main(argv: list[str]) -> int:
    """Rapid focus switching + guest capacity reclaim."""
    if len(argv) != 2:
        print("usage: m23_4_stress_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)

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
    assert "m23.4-C:" in blob_u, blob_u
    assert "used_after=0" in blob_u, blob_u

    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())
    r2 = subprocess.run(
        [
            str(vm),
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            "m23-stab-stress",
            "--max-steps",
            "900000",
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=300,
        cwd=str(ROOT),
    )
    blob = (r2.stdout or "") + (r2.stderr or "")
    assert r2.returncode == 0, blob
    assert "guest halted" in (r2.stdout or ""), blob
    assert "[haeun-fault]" not in blob, blob
    calc = int(re.search(r"spawn calc pid=(\d+)", blob).group(1))
    edit = int(re.search(r"spawn edit pid=(\d+)", blob).group(1))
    for m in re.finditer(r"\[haeun-evt\] key pid=(\d+) sc=0x1e down", blob, re.I):
        assert int(m.group(1)) == calc, blob
    for m in re.finditer(r"\[haeun-evt\] key pid=(\d+) sc=0x30 down", blob, re.I):
        assert int(m.group(1)) == edit, blob

    runner = out / "process_sdk.hxf"
    runner.write_bytes(build_process_sdk.build())
    cap = out / "app_win_cap.hxf"
    cap.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_win_cap.s")
    )
    r3 = subprocess.run(
        [str(vm), str(runner), "--app", str(cap), "--gui", "--desktop"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob3 = (r3.stdout or "") + (r3.stderr or "")
    assert r3.returncode == 0 and "guest halted" in (r3.stdout or ""), blob3
    assert "[haeun-fault]" not in blob3, blob3

    print("m23.4 stress tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
