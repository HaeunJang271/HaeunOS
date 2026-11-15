#!/usr/bin/env python3
"""Repeatability harness for M16 reliability (M14/M15 + reliability unit tests)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
REPEATS = 3


def run(cmd: list[str]) -> int:
    print("+", " ".join(cmd), flush=True)
    return subprocess.call(cmd, cwd=str(ROOT))


def exe(name: str) -> Path:
    p = BUILD / name
    if p.exists():
        return p
    pe = BUILD / f"{name}.exe"
    if pe.exists():
        return pe
    raise SystemExit(f"missing binary: {name}")


def main() -> int:
    vm = sys.argv[1] if len(sys.argv) > 1 else str(exe("haeun-vm"))
    suites = [
        [str(exe("m16-reliability-tests"))],
        [str(exe("m15-isolation-tests"))],
        [str(exe("m14-app-env-tests"))],
        [sys.executable, str(ROOT / "tests" / "m14_process_apps_tests.py"), vm],
    ]
    ok = 0
    total = 0
    for suite in suites:
        for i in range(REPEATS):
            total += 1
            rc = run(suite)
            if rc != 0:
                print(f"FAIL repeat {i + 1}/{REPEATS}: {suite} rc={rc}")
                print(f"m16_repeat_tests: {ok}/{total} passed")
                return 1
            ok += 1
    print(f"m16_repeat_tests: {ok}/{total} passed ({REPEATS}x each suite)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
