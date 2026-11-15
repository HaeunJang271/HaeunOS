#!/usr/bin/env python3
"""M14: launcher + three independent app HXFs under --apps."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_apps  # noqa: E402
import haeun_toolchain as tc  # noqa: E402


def _require_clean_run(blob: str, stdout: str) -> None:
    """Fail on user/kernel fault diagnostics or missing spawn markers."""
    if "[haeun-fault]" in blob:
        raise AssertionError(f"unexpected fault diagnostic:\n{blob}")
    assert "guest halted" in stdout, stdout
    assert "apps: registered calc" in stdout, stdout
    assert "apps: registered edit" in stdout, stdout
    assert "apps: registered files" in stdout, stdout
    assert "app-env: spawn calc" in stdout, stdout
    assert "app-env: spawn edit" in stdout, stdout
    assert "app-env: spawn files" in stdout, stdout


def _verify_app_hxf(name: str, data: bytes) -> None:
    """Static checks: entry, size, disassembly, relocated branches in-range."""
    assert data[:4] == b"HXF1", name
    entry = int.from_bytes(data[16:20], "little")
    assert entry == 0x00108000, (name, hex(entry))
    code = data[96:]
    assert 0 < len(code) <= 4096, (name, len(code))
    lines = tc.dump_hxf(data)
    assert lines, name
    for place in range(0, len(code), 4):
        word = int.from_bytes(code[place : place + 4], "little")
        opc = word >> 26
        if opc not in (10, 11, 12):
            continue
        imm = word & 0x3FFFF
        if imm >= (1 << 17):
            imm -= 1 << 18
        target = place + 4 + imm * 4
        assert 0 <= target <= len(code) - 4, (name, hex(place), hex(target))


def main(argv: list[str]) -> int:
    """Build launcher + apps, run haeun-vm --apps, require clean execution."""
    if len(argv) != 2:
        print("usage: m14_process_apps_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    (out / "process_apps.hxf").write_bytes(build_process_apps.build())
    for name, src in (
        ("app_calc.hxf", "app_calc.s"),
        ("app_edit.hxf", "app_edit.s"),
        ("app_files.hxf", "app_files.s"),
    ):
        data = build_app_hxf.build_app(ROOT / "userland" / src)
        _verify_app_hxf(name, data)
        (out / name).write_bytes(data)

    result = subprocess.run(
        [str(vm), str(out / "process_apps.hxf"), "--apps"],
        capture_output=True,
        text=True,
        check=False,
        timeout=120,
        cwd=str(ROOT),
    )
    blob = (result.stdout or "") + (result.stderr or "")
    if result.returncode != 0:
        print(blob, file=sys.stderr)
        return 1
    try:
        _require_clean_run(blob, result.stdout)
    except AssertionError as exc:
        print(exc, file=sys.stderr)
        return 1
    print("m14 process apps tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
