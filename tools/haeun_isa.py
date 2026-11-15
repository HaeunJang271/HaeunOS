#!/usr/bin/env python3
"""Load authoritative HAEUN-32 ISA metadata and disassemble instruction words."""

from __future__ import annotations

import json
from pathlib import Path

_JSON = Path(__file__).resolve().parent / "isa" / "haeun_isa.json"


def load_isa(path: Path | None = None) -> dict:
    """Return the parsed ISA metadata document."""
    return json.loads((path or _JSON).read_text(encoding="utf-8"))


def build_tables(data: dict | None = None):
    """
    Build assembler lookup tables from metadata.

    Returns (ops, cmp_ops, csr_ops, op_by_value, max_isa).
    """
    meta = data or load_isa()
    ops = {}
    op_by_value = {}
    for entry in meta["opcodes"]:
        op_by_value[entry["op"]] = entry
        if entry.get("mnemonic"):
            ops[entry["mnemonic"]] = entry["op"]
    cmp_ops = {row["mnemonic"]: row["cond"] for row in meta["cmp_mnemonics"]}
    csr_ops = {row["mnemonic"]: row["funct"] for row in meta["csr_mnemonics"]}
    return ops, cmp_ops, csr_ops, op_by_value, int(meta["max_isa"])


_OPS, _CMP_OPS, _CSR_OPS, _OP_BY_VALUE, MAX_ISA = build_tables()
OPS = _OPS
CMP_OPS = _CMP_OPS
CSR_OPS = _CSR_OPS
_CMP_BY_COND = {cond: name for name, cond in CMP_OPS.items()}
_CSR_BY_FUNCT = {funct: name for name, funct in CSR_OPS.items()}


def sign_extend(value: int, bits: int) -> int:
    """Sign-extend a bits-wide two's-complement field to Python int."""
    value &= (1 << bits) - 1
    sign = 1 << (bits - 1)
    if value & sign:
        return value - (1 << bits)
    return value


def disassemble_word(word: int, addr: int = 0, symbols: dict | None = None) -> str:
    """
    Format one little-endian instruction word as assembly text.

    Unknown or reserved encodings return a diagnostic string starting with '?'.
    """
    symbols = symbols or {}
    op = (word >> 26) & 0x3F
    rd = (word >> 22) & 0xF
    ra = (word >> 18) & 0xF
    rb = (word >> 14) & 0xF
    imm = sign_extend(word & 0x3FFFF, 18)
    entry = _OP_BY_VALUE.get(op)
    if entry is None:
        return f"? invalid opcode 0x{op:02x} @ 0x{addr:08x} word=0x{word:08x}"

    enc = entry["encoding"]
    if enc == "none":
        if word & 0x03FFFFFF:
            return f"? reserved bits set in {entry['mnemonic']} @ 0x{addr:08x}"
        return entry["mnemonic"]

    if enc == "rrr":
        if word & 0x3FFF:
            return f"? reserved bits set in {entry['mnemonic']} @ 0x{addr:08x}"
        return f"{entry['mnemonic']} r{rd}, r{ra}, r{rb}"

    if enc == "rri":
        return f"{entry['mnemonic']} r{rd}, r{ra}, {imm}"

    if enc == "load":
        return f"{entry['mnemonic']} r{rd}, {imm}(r{ra})"

    if enc == "store":
        return f"{entry['mnemonic']} r{rd}, {imm}(r{ra})"

    if enc == "branch":
        target = addr + 4 + imm * 4
        label = symbols.get(target, f"{imm:+d}")
        return f"{entry['mnemonic']} r{rd}, r{ra}, {label}"

    if enc == "jal":
        if word & 0x003C0000:
            return f"? reserved bits set in jal @ 0x{addr:08x}"
        target = addr + 4 + imm * 4
        label = symbols.get(target, f"{imm:+d}")
        return f"jal r{rd}, {label}"

    if enc == "cmp":
        cond = (word >> 11) & 7
        if (word & 0x7FF) != 0 or cond > 5:
            return f"? invalid cmp encoding @ 0x{addr:08x}"
        name = _CMP_BY_COND.get(cond)
        if name is None:
            return f"? invalid cmp cond @ 0x{addr:08x}"
        return f"{name} r{rd}, r{ra}, r{rb}"

    if enc == "csr":
        funct = (word >> 12) & 3
        csr = word & 0xFFF
        if funct == 0 or (word & 0x3C000) != 0:
            return f"? invalid csr encoding @ 0x{addr:08x}"
        name = _CSR_BY_FUNCT.get(funct)
        if name is None:
            return f"? invalid csr funct @ 0x{addr:08x}"
        return f"{name} r{rd}, {csr}, r{ra}"

    return f"? unsupported encoding {enc} @ 0x{addr:08x}"


def disassemble_bytes(data: bytes, base: int = 0, symbols: dict | None = None) -> list[str]:
    """Disassemble a little-endian instruction stream into text lines."""
    lines = []
    symbols = symbols or {}
    for offset in range(0, len(data) - len(data) % 4, 4):
        word = int.from_bytes(data[offset : offset + 4], "little")
        addr = base + offset
        label = symbols.get(addr)
        prefix = f"0x{addr:08x}: "
        text = disassemble_word(word, addr, symbols)
        if label:
            lines.append(f"{prefix}{label}:")
            lines.append(f"{' ' * len(prefix)}{text}")
        else:
            lines.append(f"{prefix}{text}")
    if len(data) % 4:
        lines.append(f"? trailing {len(data) % 4} unaligned byte(s) at end of image")
    return lines
