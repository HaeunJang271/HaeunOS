#!/usr/bin/env python3
"""M21.3: persist runtime survives past 10M; non-persist budget retained."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402
import build_process_fault  # noqa: E402
import build_process_launch  # noqa: E402

HISTORIC_BUDGET = 10_000_000


def build_apps(out: Path) -> None:
    """Build builtin apps for --apps registration."""
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


def run_vm(vm: Path, args: list[str], timeout: int = 600) -> tuple[int, str, str]:
    """Run haeun-vm; return (code, stdout, stderr)."""
    r = subprocess.run(
        [str(vm), *args],
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(ROOT),
    )
    return r.returncode, r.stdout or "", r.stderr or ""


def halted_steps(stdout: str) -> int:
    """Parse halted instruction count."""
    m = re.search(r"guest halted after (\d+) instructions", stdout)
    assert m, stdout
    return int(m.group(1))


def main(argv: list[str]) -> int:
    """M21.3 automated acceptance."""
    if len(argv) != 2:
        print("usage: m21_3_persistent_runtime_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())

    # 1–5: stress past historic 10M — key after 10.5M, Esc at 11M.
    code, so, se = run_vm(
        vm,
        [
            str(desk),
            "--apps",
            "--persist",
            "--persist-stress",
            "--input",
            "--max-steps",
            "12000000",
        ],
    )
    blob = so + se
    assert code == 0, blob
    assert "persist: run_budget=" in so, blob
    assert "guest halted" in so, blob
    assert "r1=0x00000000" in so, blob
    steps = halted_steps(so)
    assert steps > HISTORIC_BUDGET, (steps, "must survive past 10M")
    assert steps >= 11_000_000, (steps, "Esc scripted at 11M")
    # Post-10M click + Esc — no mid-WAIT spawn stealing focus.
    assert re.search(r"\[haeun-evt\] key pid=\d+ sc=0x01 down", blob, re.I), blob
    assert re.search(r"input-host:.*mice=[1-9]", blob), blob
    assert "STEP_LIMIT" not in se, blob
    assert "app-env: spawn" not in so, blob

    # 6: contained user fault still ends in guest halt (not swallowed as success).
    fault = out / "process_fault.hxf"
    fault.write_bytes(build_process_fault.build())
    code, so, se = run_vm(vm, [str(fault)], timeout=120)
    blob = so + se
    assert code == 0, blob
    assert "guest halted" in so, blob
    # Explicit ceiling still yields STEP_LIMIT (never silently ignored).
    code, so, se = run_vm(
        vm,
        [str(desk), "--apps", "--persist", "--max-steps", "50000"],
        timeout=60,
    )
    blob = so + se
    assert code != 0, blob
    assert "STEP_LIMIT" in se, blob

    # 7: non-persist default/ceiling retains STEP_LIMIT when guest never halts.
    code, so, se = run_vm(
        vm,
        [str(desk), "--apps", "--max-steps", str(HISTORIC_BUDGET)],
        timeout=180,
    )
    blob = so + se
    assert code != 0, blob
    assert "STEP_LIMIT" in se or "result 6" in se, blob
    assert "persist: run_budget=" not in so, blob

    # M20 launcher non-persist still clean-halts under default budget.
    launch = out / "process_launch.hxf"
    launch.write_bytes(build_process_launch.build())
    code, so, se = run_vm(vm, [str(launch), "--apps"], timeout=180)
    blob = so + se
    assert code == 0, blob
    assert "guest halted" in so, blob
    assert "[haeun-fault]" not in blob, blob

    print("m21_3_persistent_runtime_tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
