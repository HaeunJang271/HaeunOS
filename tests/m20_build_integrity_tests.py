#!/usr/bin/env python3
"""M20.0.1: HXF relocation + branch-target integrity for SDK/app builds."""
from __future__ import annotations

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_app_hxf as bah  # noqa: E402
import haeun_toolchain as tc  # noqa: E402


def _imm18(word: int) -> int:
    imm = word & 0x3FFFF
    return imm - (1 << 18) if imm >= (1 << 17) else imm


def test_pool_pattern_relocated() -> None:
    """jal rX, label ; .word CONST ; label: must skip the literal."""
    src = """\
.isa 4
.global user_init
user_init:
    jal r1, pool
    .word 0x12345678
pool:
    lw r2, 0(r1)
    addi r3, r0, 0
    # self-check: r2 must equal constant (loaded via jal link addr)
    jal r4, expect
    .word 0x12345678
expect:
    lw r4, 0(r4)
    bne r2, r4, bad
    addi r2, r0, 0
    addi r1, r0, 0
    ecall
    halt
bad:
    addi r2, r0, 0xEE
    addi r1, r0, 0
    ecall
    halt
"""
    hxf = bah.build_app_text(src, label="pool")
    code = hxf[96:]
    # First instruction is jal; imm must be +1 word (skip .word)
    word0 = struct.unpack_from("<I", code, 0)[0]
    assert word0 >> 26 == 12, hex(word0)
    assert _imm18(word0) == 1, _imm18(word0)
    # Unresolved external must fail
    bad = """\
.isa 4
.global user_init
.global missing_sym
user_init:
    jal r15, missing_sym
"""
    try:
        bah.build_app_text(bad, label="unresolved")
        raise AssertionError("expected unresolved reject")
    except tc.ToolchainError as exc:
        assert "unresolved" in str(exc).lower() or "undefined" in str(exc).lower()


def test_sdk_examples_disasm(vm_unused: Path | None = None) -> None:
    """Each SDK example must build with applied PCREL and valid entry."""
    del vm_unused
    for name in ("hello", "calculator", "file_viewer", "drawing", "procinfo"):
        src = ROOT / "sdk" / "examples" / name / "src" / "app.s"
        hxf = bah.build_app_with_libapp(src)
        assert hxf[:4] == b"HXF1"
        entry = int.from_bytes(hxf[16:20], "little")
        assert entry == 0x00108000, (name, hex(entry))
        code = hxf[96:]
        assert 0 < len(code) <= 4096
        # At least one jal/beq/bne with nonzero imm when pools/calls exist
        lines = tc.dump_hxf(hxf)
        text = "\n".join(lines)
        assert "0x00108000" in text
        # calculator uses literal pools — first jal must not fall into .word
        if name == "calculator":
            w0 = struct.unpack_from("<I", code, 0)[0]
            # user_init starts with jal app_getpid — imm != 0
            assert w0 >> 26 == 12
            assert _imm18(w0) != 0, "calculator first jal still reloc=0"


def test_reject_raw_hobj_pack_semantics() -> None:
    """Assembler output before link must still have zero immediates at reloc sites."""
    src = bah.combine_sources(
        [
            ROOT / "sdk" / "examples" / "hello" / "src" / "app.s",
            ROOT / "userland" / "libapp.s",
        ]
    )
    parsed = tc.parse_hobj(tc.assemble(src))
    assert parsed["relocs"], "hello+libapp must emit relocations"
    text = parsed["sections"][0]["data"]
    for reloc in parsed["relocs"]:
        word = struct.unpack_from("<I", text, reloc["offset"])[0]
        assert (word & 0x3FFFF) == 0, "assembler must leave reloc imm=0"
    # Linked app must patch those sites
    hxf = bah.build_app_text(src)
    code = hxf[96:]
    patched = 0
    for reloc in parsed["relocs"]:
        word = struct.unpack_from("<I", code, reloc["offset"])[0]
        if (word & 0x3FFFF) != 0:
            patched += 1
    assert patched > 0, "expected at least one nonzero relocated immediate"


def main(argv: list[str]) -> int:
    """Run build-integrity checks (VM path optional / unused)."""
    test_pool_pattern_relocated()
    test_reject_raw_hobj_pack_semantics()
    test_sdk_examples_disasm(Path(argv[1]) if len(argv) > 1 else None)
    print("m20 build integrity tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
