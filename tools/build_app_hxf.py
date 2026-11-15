#!/usr/bin/env python3
"""Build a standalone user-app HXF (entry/load VA 0x00108000, ISA 4)."""
from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import haeun_toolchain as tc  # noqa: E402

USER_VA = 0x00108000
MAX_CODE = 4096


def combine_sources(sources: list[Path]) -> str:
    """Concatenate ASM sources; keep a single .isa from the first file."""
    if not sources:
        raise tc.ToolchainError("no sources")
    chunks: list[str] = []
    for i, path in enumerate(sources):
        text = path.read_text(encoding="utf-8")
        if i > 0:
            text = re.sub(r"(?m)^\s*\.isa\s+\d+\s*$", "", text)
        chunks.append(f"# ---- {path.name} ----\n{text}")
    return "\n".join(chunks)


def _signed_imm18(raw: int) -> int:
    """Decode a PCREL18 immediate field as signed."""
    imm = raw & 0x3FFFF
    if imm >= (1 << 17):
        imm -= 1 << 18
    return imm


def verify_relocated_branches(code: bytes, text_symbols: dict[str, int]) -> None:
    """
    Reject unresolved-looking branches and out-of-image targets.

    Every BEQ/BNE/JAL immediate must land on a 4-byte-aligned offset inside
    the code image. Symbol-named targets (when present) must match.
    """
    if len(code) % 4:
        raise tc.ToolchainError("code size not 4-aligned")
    for place in range(0, len(code), 4):
        word = struct.unpack_from("<I", code, place)[0]
        opc = word >> 26
        if opc not in (10, 11, 12):
            continue
        imm = _signed_imm18(word)
        target = place + 4 + imm * 4
        if target & 3:
            raise tc.ToolchainError(
                f"branch at 0x{place:x} has misaligned target 0x{target:x}"
            )
        if target < 0 or target + 4 > len(code):
            raise tc.ToolchainError(
                f"branch at 0x{place:x} target 0x{target:x} outside code image"
            )
    # user_init must exist at offset 0 when declared.
    if "user_init" in text_symbols and text_symbols["user_init"] != 0:
        raise tc.ToolchainError(
            f"user_init must be at .text+0 (got {text_symbols['user_init']})"
        )


def wrap_app_hxf(code: bytes, isa_version: int = 4) -> bytes:
    """Emit a one-segment app HXF v1 at USER_VA (entry = load VA)."""
    if len(code) > MAX_CODE:
        raise tc.ToolchainError(f"code exceeds 4KiB ({len(code)})")
    if len(code) == 0 or len(code) % 4:
        raise tc.ToolchainError("app code must be nonempty and 4-aligned")
    file_off = 96
    total = file_off + len(code)
    out = bytearray(total)
    out[0:4] = b"HXF1"
    out[4:6] = (64).to_bytes(2, "little")
    out[6:8] = (1).to_bytes(2, "little")
    out[8:12] = isa_version.to_bytes(4, "little")
    out[16:20] = USER_VA.to_bytes(4, "little")
    out[20:24] = (64).to_bytes(4, "little")
    out[24:26] = (1).to_bytes(2, "little")
    out[26:28] = (32).to_bytes(2, "little")
    out[28:32] = total.to_bytes(4, "little")
    out[64:68] = (1).to_bytes(4, "little")
    out[68:72] = (7).to_bytes(4, "little")  # R|W|X
    out[72:76] = file_off.to_bytes(4, "little")
    out[76:80] = USER_VA.to_bytes(4, "little")
    out[80:84] = len(code).to_bytes(4, "little")
    out[84:88] = len(code).to_bytes(4, "little")
    out[88:92] = (4).to_bytes(4, "little")
    out[file_off:] = code
    return bytes(out)


def build_app_text(source_text: str, label: str = "<stdin>") -> bytes:
    """
    Assemble source and link via shared PCREL18 semantics into an app HXF.

    Does not treat raw assembler section bytes as final executable code.
    """
    obj = tc.assemble(source_text)
    isa_version, areas, text_symbols = tc.link_relocated_sections([obj])
    if areas[".data"]:
        raise tc.ToolchainError(f"{label}: app HXF v1 does not carry .data yet")
    code = bytes(areas[".text"])
    if "user_init" not in text_symbols:
        raise tc.ToolchainError(f"{label}: missing user_init")
    if text_symbols["user_init"] != 0:
        raise tc.ToolchainError(
            f"{label}: user_init must be at .text+0 (got {text_symbols['user_init']})"
        )
    verify_relocated_branches(code, text_symbols)
    return wrap_app_hxf(code, isa_version=isa_version)


def build_app(source: Path, extras: list[Path] | None = None) -> bytes:
    """Assemble source (+ optional libs) into an app HXF."""
    paths = [source] + list(extras or [])
    return build_app_text(combine_sources(paths), label=source.name)


def build_app_with_libapp(source: Path, libapp: Path | None = None) -> bytes:
    """Build app linked with libapp.s (default: userland/libapp.s)."""
    lib = libapp if libapp is not None else ROOT / "userland" / "libapp.s"
    return build_app(source, [lib])


def main(argv: list[str] | None = None) -> int:
    """CLI: build_app_hxf.py <app.s> <out.hxf> [--lib path.s ...]"""
    args = argv if argv is not None else sys.argv[1:]
    if len(args) < 2:
        print(
            "usage: build_app_hxf.py <app.s> <out.hxf> [--with-libapp] [--lib x.s]",
            file=sys.stderr,
        )
        return 2
    src = Path(args[0])
    out = Path(args[1])
    extras: list[Path] = []
    i = 2
    while i < len(args):
        if args[i] == "--with-libapp":
            extras.append(ROOT / "userland" / "libapp.s")
            i += 1
        elif args[i] == "--lib" and i + 1 < len(args):
            extras.append(Path(args[i + 1]))
            i += 2
        else:
            print(f"unknown arg {args[i]}", file=sys.stderr)
            return 2
    out.parent.mkdir(parents=True, exist_ok=True)
    try:
        out.write_bytes(build_app(src, extras))
    except tc.ToolchainError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
