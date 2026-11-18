#!/usr/bin/env python3
"""M22.3: guest-level interactive Calculator / Editor / Files verification (D4)."""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf  # noqa: E402
import build_process_desktop  # noqa: E402

FB_W, FB_H = 320, 180
# 8×8 bitmaps (bit0=left), subset of src/gfx_font.c ASCII set.
FONT8: dict[str, tuple[int, ...]] = {
    "0": (0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00),
    "1": (0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00),
    "2": (0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00),
    "3": (0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00),
    "4": (0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00),
    "5": (0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00),
    "7": (0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00),
    "8": (0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00),
    "9": (0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00),
    "a": (0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00),
    "b": (0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00),
    " ": (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    "c": (0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00),
    "l": (0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00),
    "F": (0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00),
    "i": (0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00),
    "e": (0x00, 0x00, 0x1E, 0x33, 0x3F, 0x03, 0x1E, 0x00),
    "s": (0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00),
}


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


def read_ppm_rgb(path: Path, x: int, y: int) -> tuple[int, int, int]:
    """Return RGB at (x,y) from binary P6 PPM."""
    data = path.read_bytes()
    parts = data.split(b"\n", 3)
    assert parts[0] == b"P6", path
    body = parts[3]
    off = (y * FB_W + x) * 3
    return body[off], body[off + 1], body[off + 2]


def is_ink(rgb: tuple[int, int, int], mode: str) -> bool:
    """True if pixel looks like DRAW_TEXT foreground for the given app."""
    r, g, b = rgb
    if mode == "calc":
        return g >= 200 and r >= 120 and b >= 120
    if mode == "edit":
        return g >= 180 and b >= 200
    if mode == "files":
        return r + g + b >= 500
    return False


def extract_glyph(path: Path, x: int, y: int, mode: str) -> tuple[int, ...]:
    """Read an 8×8 ink bitmap (bit0=left) from the framebuffer PPM."""
    rows: list[int] = []
    for row in range(8):
        bits = 0
        for col in range(8):
            if is_ink(read_ppm_rgb(path, x + col, y + row), mode):
                bits |= 1 << col
        rows.append(bits)
    return tuple(rows)


def match_glyph(bits: tuple[int, ...]) -> str | None:
    """Map bitmap to a known FONT8 character."""
    for ch, rows in FONT8.items():
        if rows == bits:
            return ch
    return None


def read_text(path: Path, x: int, y: int, n: int, mode: str) -> str:
    """OCR n consecutive 8×8 glyphs starting at (x,y)."""
    out = []
    for i in range(n):
        ch = match_glyph(extract_glyph(path, x + i * 8, y, mode))
        assert ch is not None, (x + i * 8, y, extract_glyph(path, x + i * 8, y, mode))
        out.append(ch)
    return "".join(out)


def run_vm(vm: Path, args: list[str], timeout: int = 300) -> tuple[int, str, str]:
    """Run haeun-vm and capture stdout/stderr."""
    r = subprocess.run(
        [str(vm), *args],
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
        cwd=str(ROOT),
    )
    return r.returncode, r.stdout or "", r.stderr or ""


def run_script(
    vm: Path, desk: Path, script: str, ppm: Path, max_steps: int
) -> tuple[int, str]:
    """Run desktop with a named M22.3 input script and dump FB."""
    code, so, se = run_vm(
        vm,
        [
            str(desk),
            "--apps",
            "--persist",
            "--input-script",
            script,
            "--max-steps",
            str(max_steps),
            "--fb-dump",
            str(ppm),
        ],
    )
    return code, so + se


def assert_spawn_key(blob: str, name: str, sc: str) -> None:
    """Require spawn log and a KEY_DOWN for the given scancode."""
    assert f"app-env: spawn {name}" in blob, blob
    assert re.search(rf"\[haeun-evt\] key pid=\d+ sc={sc} down", blob, re.I), blob


def main(argv: list[str]) -> int:
    """Execute M22.3 interactive guest app checks."""
    if len(argv) != 2:
        print("usage: m22_3_interactive_app_tests.py <haeun-vm>", file=sys.stderr)
        return 2
    vm = Path(argv[1])
    out = ROOT / "build"
    out.mkdir(parents=True, exist_ok=True)
    build_apps(out)
    desk = out / "process_desktop.hxf"
    desk.write_bytes(build_process_desktop.build())

    # --- Calculator arithmetic (freeze with --max-steps; result still painted) ---
    # Budget must cover spawn + several paint/compose cycles after the last key.
    cases = (
        ("calc-add", "3", 300000),
        ("calc-sub", "5", 300000),
        ("calc-mul", "4", 300000),
        ("calc-div", "4", 300000),
        ("calc-bs", "1", 300000),
    )
    for script, expect, steps in cases:
        ppm = out / f"m22_3_{script}.ppm"
        code, blob = run_script(vm, desk, script, ppm, steps)
        assert "wrote framebuffer PPM" in blob, blob
        assert "[haeun-fault]" not in blob, blob
        assert "app-env: spawn calc" in blob, blob
        assert ppm.is_file(), ppm
        # Display origin: calc win (8,18) + draw_num (8,40) → (16,58)
        got = read_text(ppm, 16, 58, len(expect), "calc")
        assert got == expect, (script, got, expect, code)

    # Calculator Esc exit → desktop resumes → clean shutdown
    ppm = out / "m22_3_calc_esc.ppm"
    code, blob = run_script(vm, desk, "calc-esc", ppm, 300000)
    assert code == 0, blob
    assert "guest halted" in blob and "r1=0x00000000" in blob, blob
    assert "app-env: spawn calc" in blob, blob

    # --- Editor: "ab 1" then Backspace → "ab " ---
    ppm = out / "m22_3_edit_type.ppm"
    code, blob = run_script(vm, desk, "edit-type", ppm, 300000)
    assert "app-env: spawn edit" in blob, blob
    assert ppm.is_file(), ppm
    # edit win (164,18) + text (8,32) → (172,50)
    got = read_text(ppm, 172, 50, 3, "edit")
    assert got == "ab ", (got, blob)

    # --- Files: registry index 2, list painted, Esc closes ---
    ppm = out / "m22_3_files_list.ppm"
    code, blob = run_script(vm, desk, "files-list", ppm, 300000)
    assert_spawn_key(blob, "files", "0x04")
    assert "app-env: spawn files" in blob, blob
    assert "app-env: spawn filemgr" not in blob, blob
    # files win (80,18) + title (8,8) / name (8,28) → (88,26)/(88,46)
    title = read_text(ppm, 88, 26, 5, "files")
    assert title == "Files", title
    first = read_text(ppm, 88, 46, 4, "files")
    assert first == "calc", first

    ppm = out / "m22_3_files_esc.ppm"
    code, blob = run_script(vm, desk, "files-esc", ppm, 300000)
    assert code == 0 and "guest halted" in blob, blob
    assert "r1=0x00000000" in blob, blob

    # --- Lifecycle / persistence: repeated launches; desktop stays alive ---
    ppm = out / "m22_3_life.ppm"
    code, blob = run_script(vm, desk, "life-cycle", ppm, 800000)
    assert code == 0, blob
    assert "guest halted" in blob and "r1=0x00000000" in blob, blob
    assert blob.count("app-env: spawn calc") >= 2, blob
    assert "app-env: spawn edit" in blob, blob
    assert "app-env: spawn files" in blob, blob
    m = re.search(r"guest halted after (\d+) instructions", blob)
    assert m and int(m.group(1)) >= 560000, blob

    print("m22_3_interactive_app_tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
