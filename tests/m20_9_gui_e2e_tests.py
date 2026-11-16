#!/usr/bin/env python3
"""M20.9: GUI E2E — sequential app launch, input markers, FB PPM sample."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_launch  # noqa: E402
import build_process_sdk  # noqa: E402


def read_ppm_rgb(path: Path, x: int, y: int, width: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from binary P6 PPM."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    body = parts[3]
    off = (y * width + x) * 3
    return body[off], body[off + 1], body[off + 2]


def build_apps(out: Path) -> None:
    """Build builtin --apps HXF images used by process_launch."""
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
    """Run launcher E2E + scripted input + PPM checks."""
    if len(argv) != 2:
        print("usage: m20_9_gui_e2e_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    (out / "process_launch.hxf").write_bytes(build_process_launch.build())
    ppm = out / "m20_9_launch.ppm"

    r = subprocess.run(
        [
            str(vm),
            str(out / "process_launch.hxf"),
            "--apps",
            "--fb-dump",
            str(ppm),
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    if r.returncode != 0:
        print(blob, file=sys.stderr)
        return 1
    assert "guest halted" in r.stdout, blob
    assert "r1=0x00000001" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    assert "app-env: spawn calc" in r.stdout, blob
    assert "app-env: spawn edit" in r.stdout, blob
    assert "app-env: spawn filemgr" in r.stdout or "app-env: spawn app_filemgr" in r.stdout, blob
    assert "app-env: spawn NOSPC" in blob, blob
    assert ppm.is_file(), "missing PPM"
    # Launcher paint_ok fill 0x00004020 → dark green-ish in last compose region.
    # Sample near (8,10) where OK glyph area is drawn on the launcher window.
    r8, g8, b8 = read_ppm_rgb(ppm, 8, 10, 64)
    assert g8 >= r8 and g8 >= b8, (r8, g8, b8, "expected greenish OK fill")

    # Keyboard: m20 script feeds sc=0x1E (A); evt path must log delivery.
    # Final r1 is sdk_runner EXIT(1), not app exit code — do not confuse them.
    sdk = out / "process_sdk.hxf"
    sdk.write_bytes(build_process_sdk.build())
    echo = out / "app_input_echo.hxf"
    echo.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_input_echo.s")
    )
    r = subprocess.run(
        [str(vm), str(sdk), "--app", str(echo), "--gui", "--desktop", "--input"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    assert "guest halted" in r.stdout, blob
    assert "[haeun-fault]" not in blob, blob
    assert re.search(r"keys=[1-9]", blob), blob
    assert re.search(r"\[haeun-evt\] key pid=\d+ sc=0x1e down", blob, re.I), blob

    # Mouse: scripted demo must post at least one mouse event.
    mouse = out / "app_mouse_trk.hxf"
    mouse.write_bytes(
        build_app_hxf.build_app_with_libapp(ROOT / "userland" / "app_mouse_trk.s")
    )
    r = subprocess.run(
        [str(vm), str(sdk), "--app", str(mouse), "--gui", "--desktop", "--input"],
        capture_output=True,
        text=True,
        check=False,
        timeout=180,
        cwd=str(ROOT),
    )
    blob = (r.stdout or "") + (r.stderr or "")
    assert r.returncode == 0, blob
    m = re.search(r"mice=(\d+)", blob)
    assert m and int(m.group(1)) > 0, blob

    print("m20.9 gui e2e tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
