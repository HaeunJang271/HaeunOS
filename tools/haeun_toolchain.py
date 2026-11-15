#!/usr/bin/env python3
"""Deterministic HAEUN-32 assembler and HOBJ linker (Python 3, no dependencies)."""

import argparse
import re
import struct
import sys
from pathlib import Path

from haeun_isa import CMP_OPS, CSR_OPS, MAX_ISA, OPS, disassemble_bytes

HOBJ_HEADER, HOBJ_SECTION, HOBJ_SYMBOL, HOBJ_RELOC = 40, 28, 24, 16
HXF_HEADER, HXF_SEGMENT = 64, 32
HXF_BASE, USER_LIMIT = 0x00100000, 0xC0000000
STACK_BASE = 0x0FF00000
UNDEFINED = 0xFFFFFFFF
GLOBAL_BIT = 0x80000000
LABEL = re.compile(r"^[A-Za-z_.$][A-Za-z0-9_.$]*$")
INTEGER = re.compile(r"^[+-]?(?:0[xX][0-9a-fA-F]+|[0-9]+)$")
REGISTER = re.compile(r"^[rR](\d+)$")
MEMORY = re.compile(r"^(.+)\s*\(\s*([rR]\d+)\s*\)$")
C_TOKEN = re.compile(r"<=|>=|==|!=|<|>|0[xX][0-9a-fA-F]+|[0-9]+|[A-Za-z_][A-Za-z0-9_]*|[{}();,=+\-&|^]")


class ToolchainError(Exception):
    pass


def align_up(value, alignment):
    return (value + alignment - 1) & -alignment


def parse_int(value, bits, signed=True):
    try:
        result = int(value, 0)
    except ValueError as exc:
        raise ToolchainError(f"expected integer, got {value!r}") from exc
    lower = -(1 << (bits - 1)) if signed else 0
    upper = (1 << (bits - (1 if signed else 0))) - 1
    if not lower <= result <= upper:
        raise ToolchainError(f"{result} does not fit {'signed ' if signed else ''}{bits}-bit value")
    return result


def reg(value):
    match = REGISTER.fullmatch(value.strip())
    if not match or int(match.group(1)) > 15:
        raise ToolchainError(f"invalid register {value!r}; expected r0..r15")
    return int(match.group(1))


def split_operands(text):
    return [] if not text.strip() else [part.strip() for part in text.split(",")]


def parse_source(source):
    rows, labels, globals_ = [], {}, set()
    positions = {".text": 0, ".data": 0}
    alignments = {".text": 4, ".data": 4}
    section = ".text"
    isa_version = 1
    for line_no, original in enumerate(source.splitlines(), 1):
        line = original.split("#", 1)[0].split(";", 1)[0].strip()
        if not line:
            continue
        while True:
            match = re.match(r"^([A-Za-z_.$][A-Za-z0-9_.$]*):", line)
            if not match:
                break
            name = match.group(1)
            if name in labels:
                raise ToolchainError(f"line {line_no}: duplicate label {name}")
            labels[name] = (section, positions[section])
            line = line[match.end():].strip()
            if not line:
                break
        if not line:
            continue
        fields = line.split(None, 1)
        op = fields[0].lower()
        args = split_operands(fields[1] if len(fields) > 1 else "")
        if op == ".isa":
            if rows or labels or globals_ or len(args) != 1:
                raise ToolchainError(f"line {line_no}: .isa must appear once before labels/content")
            isa_version = parse_int(args[0], 16, signed=False)
            if isa_version < 1 or isa_version > MAX_ISA:
                raise ToolchainError(f"line {line_no}: unsupported HAEUN ISA version")
            continue
        if op in (".text", ".data"):
            if args: raise ToolchainError(f"line {line_no}: {op} takes no operands")
            section = op
            continue
        if op == ".global":
            if len(args) != 1 or not LABEL.fullmatch(args[0]):
                raise ToolchainError(f"line {line_no}: .global expects one symbol")
            if args[0] in globals_: raise ToolchainError(f"line {line_no}: duplicate .global {args[0]}")
            globals_.add(args[0])
            continue
        offset = positions[section]
        if op == ".byte":
            if not args: raise ToolchainError(f"line {line_no}: .byte needs values")
            for value in args: parse_int(value, 8)
            size = len(args)
        elif op == ".word":
            if not args: raise ToolchainError(f"line {line_no}: .word needs values")
            for value in args: parse_int(value, 32)
            size = len(args) * 4
        elif op == ".align":
            if len(args) != 1: raise ToolchainError(f"line {line_no}: .align needs one argument")
            alignment = parse_int(args[0], 32, signed=False)
            if alignment < 1 or alignment > 4096 or alignment & (alignment - 1):
                raise ToolchainError(f"line {line_no}: .align must be a power of two from 1 to 4096")
            alignments[section] = max(alignments[section], alignment)
            size = (-offset) & (alignment - 1)
        elif op in OPS or op in CMP_OPS or op in CSR_OPS or op in ("ret", "eret", "sfence"):
            if section != ".text": raise ToolchainError(f"line {line_no}: instructions are only valid in .text")
            if offset & 3: raise ToolchainError(f"line {line_no}: instruction is not 4-byte aligned")
            if (op == "jalr" or op == "ret") and isa_version < 2:
                raise ToolchainError(f"line {line_no}: {op} requires HAEUN ISA v2")
            if op in CMP_OPS and isa_version < 3:
                raise ToolchainError(f"line {line_no}: {op} requires HAEUN ISA v3")
            if (op in CSR_OPS or op == "eret" or op == "sfence") and isa_version < 4:
                raise ToolchainError(f"line {line_no}: {op} requires HAEUN ISA v4")
            size = 4
        else:
            raise ToolchainError(f"line {line_no}: unknown mnemonic/directive {op}")
        rows.append((line_no, section, offset, op, args, size))
        positions[section] += size
        if positions[section] > USER_LIMIT - HXF_BASE:
            raise ToolchainError(f"line {line_no}: section exceeds HAEUN user address range")
    if not positions[".text"]:
        raise ToolchainError("source must emit a nonempty .text section")
    if not positions[".data"] and any(section == ".data" for section, _ in labels.values()):
        raise ToolchainError("labels in .data require emitted data bytes")
    return rows, labels, globals_, alignments, positions, isa_version


def encode_instruction(op, args, line_no, offset, symbol_names, relocations, isa_version):
    opcode = OPS.get(op)
    def count(n):
        if len(args) != n: raise ToolchainError(f"line {line_no}: {op} expects {n} operand(s)")
    def word(major, rd=0, ra=0, rb=0, imm=0):
        return (major << 26) | (rd << 22) | (ra << 18) | (rb << 14) | (imm & 0x3FFFF)
    if op in CMP_OPS:
        count(3)
        if isa_version < 3: raise ToolchainError(f"line {line_no}: {op} requires HAEUN ISA v3")
        return word(15, reg(args[0]), reg(args[1]), reg(args[2]), CMP_OPS[op] << 11)
    if op in ("nop", "halt", "ecall"):
        count(0); return word(opcode)
    if op == "eret":
        count(0)
        if isa_version < 4: raise ToolchainError(f"line {line_no}: eret requires HAEUN ISA v4")
        return word(0x11)
    if op == "sfence":
        count(0)
        if isa_version < 4: raise ToolchainError(f"line {line_no}: sfence requires HAEUN ISA v4")
        return word(0x12)
    if op in CSR_OPS:
        count(3)
        if isa_version < 4: raise ToolchainError(f"line {line_no}: {op} requires HAEUN ISA v4")
        csr = parse_int(args[1], 12, signed=False)
        return word(0x10, reg(args[0]), reg(args[2]),
                    imm=(CSR_OPS[op] << 12) | csr)
    if op == "ret":
        count(0)
        if isa_version < 2: raise ToolchainError(f"line {line_no}: ret requires HAEUN ISA v2")
        return word(14, ra=15)
    if op in ("add", "sub", "and", "or", "xor"):
        count(3); return word(opcode, reg(args[0]), reg(args[1]), reg(args[2]))
    if op == "addi":
        count(3); return word(opcode, reg(args[0]), reg(args[1]), imm=parse_int(args[2], 18))
    if op == "jalr":
        count(3)
        if isa_version < 2: raise ToolchainError(f"line {line_no}: jalr requires HAEUN ISA v2")
        return word(opcode, reg(args[0]), reg(args[1]), imm=parse_int(args[2], 18))
    if op in ("lw", "sw"):
        count(2); match = MEMORY.fullmatch(args[1])
        if not match: raise ToolchainError(f"line {line_no}: expected offset(rN) addressing")
        return word(opcode, reg(args[0]), reg(match.group(2)), imm=parse_int(match.group(1).strip(), 18))
    if op in ("beq", "bne"):
        count(3); rd, ra, target = reg(args[0]), reg(args[1]), args[2]
    elif op == "jal":
        count(2); rd, ra, target = reg(args[0]), 0, args[1]
    else:
        raise ToolchainError(f"line {line_no}: unsupported instruction {op}")
    if INTEGER.fullmatch(target):
        immediate = parse_int(target, 18)
    else:
        if not LABEL.fullmatch(target): raise ToolchainError(f"line {line_no}: invalid target {target!r}")
        symbol_names.add(target)
        relocations.append((offset, target, 0))
        immediate = 0
    return word(opcode, rd, ra, imm=immediate)


def assemble(source):
    rows, labels, globals_, alignments, positions, isa_version = parse_source(source)
    sections = []
    for name, flags in ((".text", 5), (".data", 3)):
        if positions[name]:
            sections.append({"name": name, "flags": flags, "alignment": alignments[name],
                             "data": bytearray(positions[name])})
    section_index = {sec["name"]: i for i, sec in enumerate(sections)}
    relocations, names = [], set(labels)
    for line_no, sec_name, offset, op, args, size in rows:
        sec = sections[section_index[sec_name]]["data"]
        if op == ".align":
            continue
        if op == ".byte":
            for i, value in enumerate(args): sec[offset+i] = parse_int(value, 8) & 0xFF
        elif op == ".word":
            for i, value in enumerate(args): struct.pack_into("<I", sec, offset+i*4, parse_int(value, 32) & 0xFFFFFFFF)
        else:
            instruction = encode_instruction(op, args, line_no, offset, names, relocations, isa_version)
            struct.pack_into("<I", sec, offset, instruction)
    for _, name, _ in relocations:
        if name not in labels and name not in globals_:
            raise ToolchainError(f"undefined symbol {name}; declare external references with .global")
    names.update(globals_)
    if len(names) > 0xFFFF or len(relocations) > 0xFFFF:
        raise ToolchainError("HOBJ v2 symbol or relocation count exceeds 65535")
    symbol_names = sorted(names)
    sym_index = {name: i for i, name in enumerate(symbol_names)}
    section_off = HOBJ_HEADER
    symbols_off = section_off + len(sections) * HOBJ_SECTION
    relocs_off = symbols_off + len(symbol_names) * HOBJ_SYMBOL
    data_off = relocs_off + len(relocations) * HOBJ_RELOC
    section_data_offsets, cursor = {}, data_off
    for sec in sections:
        section_data_offsets[sec["name"]] = cursor
        cursor += len(sec["data"])
    file_size, data_size = cursor, cursor - data_off
    if file_size > 0xFFFFFFFF: raise ToolchainError("HOBJ v2 exceeds 4 GiB")
    out = bytearray(file_size)
    object_version = {1: 2, 2: 3, 3: 4, 4: 5}[isa_version]
    struct.pack_into("<4sHHHHHHIIIIII", out, 0, b"HOBJ", object_version, HOBJ_HEADER,
                     len(sections), len(symbol_names), len(relocations), isa_version if object_version >= 3 else 0,
                     section_off, symbols_off, relocs_off, data_off, data_size, file_size)
    for i, sec in enumerate(sections):
        name_bytes = sec["name"].encode("ascii")
        struct.pack_into("<8sIIIII", out, section_off+i*HOBJ_SECTION,
                         name_bytes+b"\0"*(8-len(name_bytes)), 1, sec["flags"],
                         sec["alignment"], section_data_offsets[sec["name"]], len(sec["data"]))
        start = section_data_offsets[sec["name"]]
        out[start:start+len(sec["data"])] = sec["data"]
    for i, name in enumerate(symbol_names):
        encoded = name.encode("ascii")
        if len(encoded) > 15: raise ToolchainError(f"symbol name too long for HOBJ v2: {name}")
        if name in labels:
            sec_name, value = labels[name]
            sec_id = section_index[sec_name] | (GLOBAL_BIT if name in globals_ else 0)
        else:
            value, sec_id = 0, UNDEFINED
        struct.pack_into("<16sII", out, symbols_off+i*HOBJ_SYMBOL,
                         encoded+b"\0", sec_id, value)
    relocs_out = []
    for offset, name, addend in relocations:
        sec_name = next(sec["name"] for sec in sections
                        if sec["name"] == ".text" and offset+4 <= len(sec["data"]))
        relocs_out.append((section_data_offsets[sec_name]+offset, name, addend))
    for i, (offset, name, addend) in enumerate(sorted(relocs_out)):
        struct.pack_into("<IHH i I", out, relocs_off+i*HOBJ_RELOC,
                         offset, sym_index[name], 1, addend, 0)
    return bytes(out)


def decode_name(raw):
    end = raw.find(b"\0")
    if end <= 0 or any(raw[end:]): raise ToolchainError("malformed HOBJ name")
    try: name = raw[:end].decode("ascii")
    except UnicodeDecodeError as exc: raise ToolchainError("non-ASCII HOBJ name") from exc
    if not LABEL.fullmatch(name): raise ToolchainError("invalid HOBJ identifier")
    return name


def parse_hobj(blob):
    if len(blob) < HOBJ_HEADER or blob[:4] != b"HOBJ": raise ToolchainError("invalid HOBJ magic/header")
    (version, header_size, section_count, symbol_count, reloc_count, reserved,
     section_off, symbols_off, relocs_off, data_off, data_size, file_size) = struct.unpack_from("<HHHHHHIIIIII", blob, 4)
    if version not in (1, 2, 3, 4, 5) or header_size != HOBJ_HEADER:
        raise ToolchainError("unsupported HOBJ version/header size")
    expected_reserved = {1: 0, 2: 0, 3: 2, 4: 3, 5: 4}[version]
    if not 1 <= section_count <= 16 or reserved != expected_reserved or file_size != len(blob):
        raise ToolchainError("invalid HOBJ counts/reserved/file size")
    if section_off != HOBJ_HEADER or symbols_off != section_off+section_count*HOBJ_SECTION or relocs_off != symbols_off+symbol_count*HOBJ_SYMBOL or data_off != relocs_off+reloc_count*HOBJ_RELOC or data_off+data_size != file_size:
        raise ToolchainError("HOBJ table offsets or ranges are inconsistent")
    sections, expected_data = [], data_off
    for i in range(section_count):
        name_raw, sec_type, flags, alignment, file_offset, size = struct.unpack_from("<8sIIIII", blob, section_off+i*HOBJ_SECTION)
        name = decode_name(name_raw)
        expected_flags = {".text": 5, ".data": 3}.get(name)
        if (version == 1 and (i != 0 or name != ".text")) or expected_flags is None or sec_type != 1 or flags != expected_flags or alignment < 1 or alignment > 4096 or alignment & (alignment-1) or file_offset != expected_data or size == 0 or file_offset+size > file_size:
            raise ToolchainError("unsupported or malformed HOBJ section")
        sections.append({"name": name, "flags": flags, "alignment": alignment,
                         "data": bytearray(blob[file_offset:file_offset+size]), "file_offset": file_offset})
        expected_data += size
    if expected_data != file_size or len({s["name"] for s in sections}) != section_count:
        raise ToolchainError("HOBJ sections do not cover the data area exactly")
    symbols, prior_name = [], ""
    for i in range(symbol_count):
        raw_name, a, b = struct.unpack_from("<16sII", blob, symbols_off+i*HOBJ_SYMBOL)
        name = decode_name(raw_name)
        if name <= prior_name: raise ToolchainError("HOBJ symbols must be unique and sorted")
        prior_name = name
        if version == 1:
            if b not in (0, 1) or (b == 1 and a > len(sections[0]["data"])) or (b == 0 and a != 0):
                raise ToolchainError("invalid HOBJ v1 symbol")
            symbols.append({"name": name, "section": 0 if b else None, "value": a,
                            "global": True, "defined": bool(b)})
        elif a == UNDEFINED:
            if b != 0: raise ToolchainError("undefined HOBJ symbol must have value zero")
            symbols.append({"name": name, "section": None, "value": 0, "global": True, "defined": False})
        else:
            if a & 0x7FFF0000 or (a & 0xFFFF) >= section_count or b > len(sections[a & 0xFFFF]["data"]):
                raise ToolchainError("invalid HOBJ v2 symbol section/value")
            symbols.append({"name": name, "section": a & 0xFFFF, "value": b,
                            "global": bool(a & GLOBAL_BIT), "defined": True})
    relocs, prior_offset = [], -1
    for i in range(reloc_count):
        offset, symbol, kind, addend, zero = struct.unpack_from("<IHH i I", blob, relocs_off+i*HOBJ_RELOC)
        if version == 1:
            source_section = 0 if offset+4 <= len(sections[0]["data"]) else None
            section_offset = offset
        else:
            source_section = next((j for j, sec in enumerate(sections)
                                   if sec["file_offset"] <= offset and offset+4 <= sec["file_offset"]+len(sec["data"])), None)
            section_offset = offset-sections[source_section]["file_offset"] if source_section is not None else 0
        if offset <= prior_offset or offset & 3 or source_section is None or symbol >= symbol_count or kind != 1 or zero:
            raise ToolchainError("invalid HOBJ relocation")
        prior_offset = offset
        relocs.append({"section": source_section, "offset": section_offset,
                       "symbol": symbol, "addend": addend})
    return {"version": version, "isa": {1:1, 2:1, 3:2, 4:3, 5:4}[version],
            "sections": sections, "symbols": symbols, "relocs": relocs}


def place_hobj_sections(objects):
    """
    Lay out HOBJ sections into concatenated .text/.data areas.

    Returns (placements, areas) matching link_hobjs placement rules.
    """
    placements, areas = {}, {".text": bytearray(), ".data": bytearray()}
    for oi, obj in enumerate(objects):
        for si, sec in enumerate(obj["sections"]):
            area = areas[sec["name"]]
            start = align_up(len(area), sec["alignment"])
            area.extend(bytes(start - len(area)))
            placements[(oi, si)] = start
            area.extend(sec["data"])
    return placements, areas


def resolve_symbol_addresses(objects, placements):
    """
    Build per-object symbol addresses and a global-name table.

    Returns (addresses, globals_by_name) used by PCREL18 application.
    """
    globals_by_name, addresses = {}, {}
    for oi, obj in enumerate(objects):
        for sym_i, sym in enumerate(obj["symbols"]):
            if not sym["defined"]:
                continue
            address_offset = placements[(oi, sym["section"])] + sym["value"]
            area_name = obj["sections"][sym["section"]]["name"]
            addresses[(oi, sym_i)] = (area_name, address_offset)
            if sym["global"]:
                if sym["name"] in globals_by_name:
                    raise ToolchainError(f"duplicate global symbol {sym['name']}")
                globals_by_name[sym["name"]] = (
                    area_name, address_offset, oi, sym["section"], sym["value"]
                )
    return addresses, globals_by_name


def apply_pcrel18_relocations(objects, placements, addresses, globals_by_name, text_area):
    """
    Apply every HOBJ PCREL18 (type 1) fixup into the linked .text area.

    Rejects unresolved symbols, non-.text targets, misaligned/out-of-range
    deltas, and relocation sites that are not BEQ/BNE/JAL. Mutates text_area.
    """
    for oi, obj in enumerate(objects):
        for reloc in obj["relocs"]:
            source = obj["sections"][reloc["section"]]
            if source["name"] != ".text":
                raise ToolchainError("PCREL18 relocation must be in .text")
            symbol = obj["symbols"][reloc["symbol"]]
            if symbol["defined"]:
                target_area, target_offset = addresses[(oi, reloc["symbol"])]
                target_object = oi
                target_section = symbol["section"]
                target_value = symbol["value"]
            else:
                if symbol["name"] not in globals_by_name:
                    raise ToolchainError(f"unresolved HOBJ symbol {symbol['name']}")
                (target_area, target_offset, target_object,
                 target_section, target_value) = globals_by_name[symbol["name"]]
            if target_area != ".text":
                raise ToolchainError("PC-relative branch/jump target must be in .text")
            sec_len = len(objects[target_object]["sections"][target_section]["data"])
            if target_value & 3 or target_value + 4 > sec_len:
                raise ToolchainError(
                    "PC-relative target is not an instruction within its section"
                )
            place = placements[(oi, reloc["section"])] + reloc["offset"]
            if place < 0 or place + 4 > len(text_area):
                raise ToolchainError("PCREL18 relocation site outside .text")
            delta = target_offset + reloc["addend"] - (place + 4)
            if delta % 4:
                raise ToolchainError("PC-relative target is not instruction aligned")
            immediate = delta // 4
            if not -(1 << 17) <= immediate < (1 << 17):
                raise ToolchainError("PC-relative relocation out of range")
            instruction = struct.unpack_from("<I", text_area, place)[0]
            if instruction >> 26 not in (10, 11, 12):
                raise ToolchainError(
                    "PCREL18 relocation targets a non-branch instruction"
                )
            struct.pack_into(
                "<I", text_area, place,
                (instruction & ~0x3FFFF) | (immediate & 0x3FFFF),
            )


def link_relocated_sections(blobs):
    """
    Place and relocate HOBJ blobs without emitting an HXF image.

    Returns (isa_version, areas, text_symbols) where text_symbols maps
    defined symbol names in .text to byte offsets within areas['.text'].
    """
    objects = [parse_hobj(blob) for blob in blobs]
    if not objects:
        raise ToolchainError("link requires at least one object")
    isa_versions = {obj["isa"] for obj in objects}
    if len(isa_versions) != 1:
        raise ToolchainError("cannot link objects with different HAEUN ISA versions")
    isa_version = next(iter(isa_versions))
    placements, areas = place_hobj_sections(objects)
    if not areas[".text"]:
        raise ToolchainError("link requires nonempty .text")
    addresses, globals_by_name = resolve_symbol_addresses(objects, placements)
    apply_pcrel18_relocations(
        objects, placements, addresses, globals_by_name, areas[".text"]
    )
    text_symbols = {}
    for oi, obj in enumerate(objects):
        for sym_i, sym in enumerate(obj["symbols"]):
            if not sym["defined"]:
                continue
            area_name, offset = addresses[(oi, sym_i)]
            if area_name != ".text":
                continue
            # Prefer the first definition; globals already rejected duplicates.
            text_symbols.setdefault(sym["name"], offset)
    return isa_version, areas, text_symbols


def link_hobjs(blobs):
    isa_version, areas, _text_symbols = link_relocated_sections(blobs)
    objects = [parse_hobj(blob) for blob in blobs]
    placements, _ = place_hobj_sections(objects)
    addresses, _ = resolve_symbol_addresses(objects, placements)

    starts = {".text": HXF_BASE, ".data": align_up(HXF_BASE+len(areas[".text"]), 4096)}
    entry_candidates = []
    for oi, obj in enumerate(objects):
        for si, sym in enumerate(obj["symbols"]):
            if sym["name"] == "_start" and sym["defined"]:
                area_name, offset = addresses[(oi, si)]
                sec = obj["sections"][sym["section"]]
                if area_name != ".text" or sym["value"] & 3 or sym["value"]+4 > len(sec["data"]):
                    raise ToolchainError("_start is not an instruction within its section")
                entry_candidates.append((area_name, offset))
    if len(entry_candidates) != 1 or entry_candidates[0][0] != ".text":
        raise ToolchainError("link requires exactly one _start symbol in .text")
    entry = starts[".text"] + entry_candidates[0][1]
    if entry & 3 or entry+4 > starts[".text"]+len(areas[".text"]):
        raise ToolchainError("_start is not an instruction in .text")

    active = [name for name in (".text", ".data") if areas[name]]
    table_end = HXF_HEADER + HXF_SEGMENT*len(active)
    payload_offsets, cursor = {}, table_end
    for name in active:
        cursor = align_up(cursor, 4)
        payload_offsets[name] = cursor
        cursor += len(areas[name])
    if (cursor > 0xFFFFFFFF or starts[".text"]+len(areas[".text"]) > STACK_BASE or
            starts[".data"]+len(areas[".data"]) > min(USER_LIMIT, STACK_BASE)):
        raise ToolchainError("linked image exceeds HAEUN address/file limits")
    out = bytearray(cursor)
    struct.pack_into("<4sHHIIIIHHI", out, 0, b"HXF1", 64, 1, isa_version, 0, entry,
                     HXF_HEADER, len(active), HXF_SEGMENT, cursor)
    for i, name in enumerate(active):
        flags = 5 if name == ".text" else 3
        struct.pack_into("<IIIIIIII", out, HXF_HEADER+i*HXF_SEGMENT,
                         1, flags, payload_offsets[name], starts[name],
                         len(areas[name]), len(areas[name]), 4, 0)
        out[payload_offsets[name]:payload_offsets[name]+len(areas[name])] = areas[name]
    return bytes(out)


def link_hobj(blob):
    """Compatibility helper for linking exactly one object."""
    return link_hobjs([blob])


def link_firmware(blobs):
    """Links ISA-matched HOBJ blobs at physical address 0 into an HFW v1 image."""
    isa_version, areas, text_symbols = link_relocated_sections(blobs)
    if "_start" not in text_symbols or text_symbols["_start"] != 0:
        raise ToolchainError("firmware _start must be at text offset 0")
    text = bytes(areas[".text"])
    data = bytes(areas[".data"])
    if data:
        raise ToolchainError("firmware v1 link does not support .data yet")
    file_size = len(text)
    memory_size = align_up(file_size, 4)
    if memory_size == 0 or memory_size > 65536:
        raise ToolchainError("firmware image size out of HFW range")
    return (struct.pack(
        "<4sHHIIIIII", b"HFW1", 1, 32, isa_version, 0, 0, file_size, memory_size, 0)
            + text)


class TinyCParser:
    """Deliberately narrow C17 integer-function parser for the first compiler milestone."""

    def __init__(self, source):
        source = re.sub(r"/\*.*?\*/|//[^\n]*", " ", source, flags=re.S)
        self.tokens = []
        cursor = 0
        for match in C_TOKEN.finditer(source):
            if source[cursor:match.start()].strip():
                raise ToolchainError(f"unsupported C token near {source[cursor:match.start()]!r}")
            self.tokens.append(match.group(0))
            cursor = match.end()
        if source[cursor:].strip(): raise ToolchainError(f"unsupported C token near {source[cursor:]!r}")
        self.index = 0

    def peek(self):
        return self.tokens[self.index] if self.index < len(self.tokens) else None

    def take(self, expected=None):
        token = self.peek()
        if token is None or (expected is not None and token != expected):
            raise ToolchainError(f"expected {expected or 'token'}, found {token or 'end of file'}")
        self.index += 1
        return token

    def check_variables(self, node, available):
        if node[0] == "var" and node[1] not in available:
            raise ToolchainError(f"unknown or not-yet-declared local {node[1]}")
        if node[0] == "unary": self.check_variables(node[2], available)
        if node[0] == "bin":
            self.check_variables(node[2], available)
            self.check_variables(node[3], available)
        if node[0] == "cmp":
            self.check_variables(node[2], available)
            self.check_variables(node[3], available)
        if node[0] == "call":
            for arg in node[2]: self.check_variables(arg, available)

    def parse_param_list(self):
        """Parse `(void)` or `(int a, ...)` and return the parameter name list."""
        self.take("(")
        params = []
        if self.peek() == "void":
            self.take("void")
        elif self.peek() != ")":
            while True:
                self.take("int")
                param = self.take()
                if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", param) or param in params:
                    raise ToolchainError(f"invalid or duplicate parameter {param}")
                params.append(param)
                if self.peek() != ",":
                    break
                self.take(",")
        self.take(")")
        if len(params) > 64:
            raise ToolchainError("compiler subset supports at most 64 int parameters")
        return params

    def parse(self):
        functions = []
        externs = {}
        while self.peek() is not None:
            if self.peek() == "extern":
                self.take("extern")
                self.take("int")
                name = self.take()
                if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                    raise ToolchainError(f"invalid function name {name}")
                if name in externs or any(fn["name"] == name for fn in functions):
                    raise ToolchainError(f"duplicate declaration of {name}")
                params = self.parse_param_list()
                self.take(";")
                externs[name] = len(params)
                continue
            self.take("int")
            name = self.take()
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                raise ToolchainError(f"invalid function name {name}")
            if name in externs:
                raise ToolchainError(f"function {name} was declared extern")
            params = self.parse_param_list()
            self.take("{")
            variables = {param: i for i, param in enumerate(params)}
            statements = []
            while self.peek() == "int":
                self.take("int"); local = self.take()
                if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", local) or local in variables:
                    raise ToolchainError(f"invalid or duplicate local variable {local}")
                if len(variables) >= 128:
                    raise ToolchainError("compiler subset supports at most 128 parameters and locals")
                self.take("="); expr = self.expression(); self.check_variables(expr, variables); self.take(";")
                variables[local] = len(variables)
                statements.append(("assign", local, expr))
            while self.peek() not in ("return", None):
                statements.append(self.statement(variables))
            if self.peek() is None:
                raise ToolchainError(f"function {name} must end with return expression;")
            self.take("return"); expr = self.expression(); self.check_variables(expr, variables); self.take(";")
            statements.append(("return", expr)); self.take("}")
            functions.append({"name": name, "params": params, "variables": variables,
                              "statements": statements})
        signatures = {function["name"]: len(function["params"]) for function in functions}
        if len(signatures) != len(functions):
            raise ToolchainError("duplicate function definition")
        for name, arity in externs.items():
            signatures[name] = arity
        has_main = "main" in signatures and signatures["main"] == 0 and "main" not in externs
        has_kmain = "kmain" in signatures and signatures["kmain"] == 0 and "kmain" not in externs
        if has_main and has_kmain:
            raise ToolchainError("program cannot define both main and kmain")
        if not has_main and not has_kmain and len(functions) == 0:
            raise ToolchainError("translation unit has no functions")
        def validate_calls(node, nested=False):
            if node[0] == "call":
                name, args = node[1], node[2]
                if name not in signatures:
                    raise ToolchainError(f"undefined function {name}")
                if name in ("main", "kmain"):
                    raise ToolchainError(f"{name} is an entry point and cannot be called")
                if len(args) != signatures[name]:
                    raise ToolchainError(f"function {name} expects {signatures[name]} argument(s)")
                if nested:
                    raise ToolchainError("function calls inside argument expressions are not yet supported")
                for arg in args:
                    validate_calls(arg, True)
            elif node[0] == "bin":
                validate_calls(node[2], nested); validate_calls(node[3], nested)
            elif node[0] == "cmp":
                validate_calls(node[2], nested); validate_calls(node[3], nested)
            elif node[0] == "unary":
                validate_calls(node[2], nested)
        def validate_statement(node):
            if node[0] == "assign":
                validate_calls(node[2])
            elif node[0] == "return":
                validate_calls(node[1])
            elif node[0] == "block":
                for child in node[1]:
                    validate_statement(child)
            elif node[0] == "if":
                validate_calls(node[1])
                validate_statement(node[2])
                if node[3] is not None:
                    validate_statement(node[3])
            elif node[0] == "while":
                validate_calls(node[1]); validate_statement(node[2])
        for function in functions:
            for statement in function["statements"]:
                validate_statement(statement)
        return {"functions": functions, "externs": externs}

    def block(self, variables):
        self.take("{"); statements = []
        while self.peek() != "}":
            if self.peek() is None: raise ToolchainError("unterminated compound statement")
            statements.append(self.statement(variables))
        self.take("}")
        return statements

    def statement(self, variables):
        if self.peek() == "{": return ("block", self.block(variables))
        if self.peek() in ("if", "while"):
            kind = self.take(); self.take("(")
            condition = self.expression(); self.take(")")
            self.check_variables(condition, variables)
            body = self.statement(variables)
            if kind == "while": return ("while", condition, body)
            otherwise = self.statement(variables) if self.peek() == "else" and self.take() else None
            return ("if", condition, body, otherwise)
        name = self.take()
        if name not in variables: raise ToolchainError(f"unknown local variable {name}")
        self.take("="); expr = self.expression(); self.check_variables(expr, variables); self.take(";")
        return ("assign", name, expr)

    def expression(self): return self.bit_or()
    def bit_or(self):
        node = self.bit_xor()
        while self.peek() == "|": self.take(); node = ("bin", "|", node, self.bit_xor())
        return node
    def bit_xor(self):
        node = self.bit_and()
        while self.peek() == "^": self.take(); node = ("bin", "^", node, self.bit_and())
        return node
    def bit_and(self):
        node = self.equality()
        while self.peek() == "&": self.take(); node = ("bin", "&", node, self.equality())
        return node
    def equality(self):
        node = self.relational()
        while self.peek() in ("==", "!="):
            op=self.take(); node=("cmp",op,node,self.relational())
        return node
    def relational(self):
        node=self.additive()
        while self.peek() in ("<", "<=", ">", ">="):
            op=self.take(); node=("cmp",op,node,self.additive())
        return node
    def additive(self):
        node = self.unary()
        while self.peek() in ("+", "-"):
            op = self.take(); node = ("bin", op, node, self.unary())
        return node
    def unary(self):
        if self.peek() == "-":
            self.take(); return ("unary", "-", self.unary())
        if self.peek() == "+": self.take(); return self.unary()
        return self.primary()
    def primary(self):
        token = self.peek()
        if token == "(":
            self.take(); expr = self.expression(); self.take(")"); return expr
        if token is None: raise ToolchainError("incomplete expression")
        if token[0].isdigit():
            self.take(); value = parse_int(token, 32)
            return ("const", value)
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", token):
            self.take()
            if self.peek() != "(": return ("var", token)
            self.take("("); args = []
            if self.peek() != ")":
                while True:
                    args.append(self.expression())
                    if self.peek() != ",": break
                    self.take(",")
            self.take(")")
            return ("call", token, args)
        raise ToolchainError(f"unexpected token in expression: {token}")


class TinyCCodegen:
    FRAME_HEADER_SIZE = 80

    @staticmethod
    def call_arities(value):
        if isinstance(value, tuple):
            if value and value[0] == "call":
                yield len(value[2])
            for child in value:
                yield from TinyCCodegen.call_arities(child)
        elif isinstance(value, list):
            for child in value:
                yield from TinyCCodegen.call_arities(child)

    @staticmethod
    def expression_node_count(value):
        kinds={"const","var","call","unary","bin","cmp"}
        count=1 if isinstance(value,tuple) and value and value[0] in kinds else 0
        if isinstance(value,(tuple,list)):
            return count+sum(TinyCCodegen.expression_node_count(child) for child in value)
        return count

    def __init__(self, functions, externs=None, isa_version=3):
        self.functions = functions
        self.externs = externs or {}
        self.variables = {}
        if isa_version not in (1, 2, 3, 4):
            raise ToolchainError(f"unsupported C ISA version {isa_version}")
        self.assembly = [f".isa {isa_version}", ".text"]
        if any(fn["name"] == "main" for fn in functions):
            self.assembly.append(".global _start")
        for function in functions:
            self.assembly.append(f".global {function['name']}")
        for name in sorted(self.externs):
            self.assembly.append(f".global {name}")
        self.free_temps = [11, 10, 9, 8]
        self.label_number = 0

    def label(self, purpose):
        result = f".LC_{purpose}_{self.label_number}"
        self.label_number += 1
        return result

    def spill_one(self, excluded=()):
        excluded = set(excluded)
        for temp_id, register in self.temp_locations.items():
            if register is not None and temp_id not in excluded:
                if temp_id not in self.temp_spill_slots:
                    slot = len(self.temp_spill_slots)
                    if slot >= self.temp_spill_capacity:
                        raise ToolchainError("internal temporary spill capacity exceeded")
                    self.temp_spill_slots[temp_id] = self.temp_spill_base+4*slot
                offset = self.temp_spill_slots[temp_id]
                self.assembly.append(f"    sw r{register}, {offset}(r13)")
                self.temp_locations[temp_id] = None
                self.free_temps.append(register)
                return
        raise ToolchainError("unable to spill a live expression temporary")

    def alloc_temp(self, excluded=()):
        if not self.free_temps:
            self.spill_one(excluded)
        register = self.free_temps.pop()
        temp_id = self.next_temp_id
        self.next_temp_id += 1
        self.temp_locations[temp_id] = register
        return ("temp", temp_id)

    def temp_register(self, value, protected=()):
        temp_id = value[1]
        register = self.temp_locations[temp_id]
        if register is None:
            if not self.free_temps:
                self.spill_one(set(protected)|{temp_id})
            register = self.free_temps.pop()
            self.assembly.append(f"    lw r{register}, {self.temp_spill_slots[temp_id]}(r13)")
            self.temp_locations[temp_id] = register
        return register

    def new_result(self, destination, operands):
        owner = next((value[1] for value in operands
                      if value[0] == "temp" and self.temp_locations[value[1]] == destination), None)
        for value in operands:
            if value[0] == "temp" and value[1] != owner:
                self.release(value)
        if owner is not None:
            del self.temp_locations[owner]
        result = self.next_temp_id
        self.next_temp_id += 1
        self.temp_locations[result] = destination
        return ("temp", result)

    def release(self, value):
        if value[0] == "temp" or (value[0]=="reg" and len(value)==3):
            temp_id=value[1] if value[0]=="temp" else value[2]
            register = self.temp_locations.pop(temp_id)
            if register is not None:
                self.free_temps.append(register)

    def load(self, value, protected=()):
        if value[0] == "reg": return value
        if value[0] == "temp": return ("reg",self.temp_register(value,protected),value[1])
        result = self.alloc_temp(excluded=protected)
        register = self.temp_locations[result[1]]
        self.assembly.append(f"    addi r{register}, r0, {value[1]}")
        return ("reg", register, result[1])

    @staticmethod
    def handle(value, loaded):
        if len(loaded) == 3: return ("temp",loaded[2])
        return value

    def produce_result(self, operands, preferred):
        owner = next((value for value in operands if value[0] == "temp" and
                      self.temp_locations[value[1]] == preferred), None)
        if owner is not None:
            return self.new_result(preferred,operands)
        result = self.alloc_temp(excluded=[value[1] for value in operands if value[0] == "temp"])
        return result

    def expression(self, node):
        kind = node[0]
        if kind == "const": return node
        if kind == "call": return self.call(node[1], node[2])
        if kind == "var":
            if node[1] not in self.variables: raise ToolchainError(f"unknown local {node[1]}")
            slot = self.variables[node[1]]
            if slot < 6: return ("reg", 2+slot)
            result = self.alloc_temp()
            register = self.temp_locations[result[1]]
            self.assembly.append(f"    lw r{register}, {self.spill_offset(slot)}(r13)")
            return result
        if kind == "unary":
            value = self.expression(node[2])
            if value[0] == "const":
                result = -value[1]
                if not -(1 << 31) <= result < (1 << 31): raise ToolchainError("signed constant overflow")
                return ("const", result)
            loaded=self.load(value); operand=self.handle(value,loaded)
            if operand[0]=="temp":
                dest=loaded[1]; result=self.new_result(dest,[operand])
            else:
                result=self.alloc_temp(); dest=self.temp_locations[result[1]]
            self.assembly.append(f"    sub r{dest}, r0, {loaded[1]}")
            return result
        left = self.expression(node[2]); right = self.expression(node[3]); op = node[1]
        if node[0] == "cmp":
            if left[0] == "const" and right[0] == "const":
                a,b=left[1],right[1]
                result={"==":a==b,"!=":a!=b,"<":a<b,"<=":a<=b,">":a>b,">=":a>=b}[op]
                return ("const",1 if result else 0)
            left_reg=self.load(left)
            left_handle=self.handle(left,left_reg)
            right_reg=self.load(right,[left_handle[1]] if left_handle[0]=="temp" else ())
            left_handle=self.handle(left,left_reg); right_handle=self.handle(right,right_reg)
            operands=[left_handle,right_handle]
            preferred=next((value[1] for value in (left_reg,right_reg)
                            if len(value)==3),None)
            if preferred is None:
                result=self.alloc_temp(excluded=[v[1] for v in operands if v[0]=="temp"])
                destination=self.temp_locations[result[1]]
            else:
                destination=preferred; result=self.produce_result(operands,destination)
            mnemonic={"==":"cmpeq","!=":"cmpne","<":"cmplt","<=":"cmple",
                      ">":"cmpgt",">=":"cmpge"}[op]
            self.assembly.append(f"    {mnemonic} r{destination}, r{left_reg[1]}, r{right_reg[1]}")
            for operand in operands:
                if operand[0]=="temp" and operand[1] in self.temp_locations and \
                        self.temp_locations[operand[1]] != destination:
                    self.release(operand)
            return result
        if left[0] == "const" and right[0] == "const":
            a, b = left[1], right[1]
            result = {"+": lambda: a+b, "-": lambda: a-b, "&": lambda: a&b,
                      "|": lambda: a|b, "^": lambda: a^b}[op]()
            if not -(1 << 31) <= result < (1 << 31): raise ToolchainError("signed constant overflow")
            return ("const", result)
        if op in ("+", "-") and right[0] == "const":
            immediate = right[1] if op == "+" else -right[1]
            if -(1 << 17) <= immediate < (1 << 17):
                source=self.load(left); operand=self.handle(left,source)
                if operand[0]=="temp":
                    destination=source[1]; result=self.new_result(destination,[operand])
                else:
                    result=self.alloc_temp(); destination=self.temp_locations[result[1]]
                self.assembly.append(f"    addi r{destination}, r{source[1]}, {immediate}")
                return result
        left_reg=self.load(left)
        left_handle=self.handle(left,left_reg)
        right_reg=self.load(right,[left_handle[1]] if left_handle[0]=="temp" else ())
        left_handle=self.handle(left,left_reg); right_handle=self.handle(right,right_reg)
        operands=[left_handle,right_handle]
        preferred=next((value[1] for value in (left_reg,right_reg) if len(value)==3),None)
        if preferred is None:
            result=self.alloc_temp(excluded=[v[1] for v in operands if v[0]=="temp"])
            destination=self.temp_locations[result[1]]
        else:
            destination=preferred; result=self.produce_result(operands,destination)
        mnemonic = {"+": "add", "-": "sub", "&": "and", "|": "or", "^": "xor"}[op]
        self.assembly.append(f"    {mnemonic} r{destination}, r{left_reg[1]}, r{right_reg[1]}")
        for operand in operands:
            if operand[0]=="temp" and operand[1] in self.temp_locations and \
                    self.temp_locations[operand[1]] != destination:
                self.release(operand)
        return result

    def call(self, name, arguments):
        extra_count = max(0, len(arguments)-6)
        extra_size = ((extra_count*4+15)//16)*16 if extra_count else 0
        for index, argument in enumerate(arguments):
            value = self.load(self.expression(argument))
            staging_offset = 44+index*4 if index < 6 else self.outgoing_offset+4*(index-6)
            self.assembly.append(f"    sw r{value[1]}, {staging_offset}(r13)")
            self.release(value)
        for index in range(6):
            self.assembly.append(f"    sw r{2+index}, {20+index*4}(r13)")
        for index in range(len(arguments)):
            if index < 6:
                self.assembly.append(f"    lw r{2+index}, {44+index*4}(r13)")
        if extra_size:
            self.assembly.append(f"    addi r1, r13, -{extra_size}")
            self.assembly.append("    cmplt r1, r1, r14")
            self.assembly.append("    bne r1, r0, .Lstkfail")
            self.assembly.append("    sw r11, 72(r13)")
            self.assembly.append(f"    addi r13, r13, -{extra_size}")
            for index in range(extra_count):
                source_offset = extra_size+self.outgoing_offset+4*index
                self.assembly.append(f"    lw r11, {source_offset}(r13)")
                self.assembly.append(f"    sw r11, {4*index}(r13)")
            self.assembly.append(f"    lw r11, {extra_size+72}(r13)")
        self.assembly.append(f"    jal r15, {name}")
        if extra_size:
            self.assembly.append(f"    addi r13, r13, {extra_size}")
        self.assembly.append("    sw r1, 68(r13)")
        for index in range(6):
            self.assembly.append(f"    lw r{2+index}, {20+index*4}(r13)")
        result = self.alloc_temp()
        register = self.temp_locations[result[1]]
        self.assembly.append(f"    lw r{register}, 68(r13)")
        return result

    def condition_branch(self, condition, target):
        value=self.expression(condition)
        if value[0]=="const":
            if value[1]!=0:self.assembly.append(f"    jal r0, {target}")
            return
        register=self.load(value)
        self.assembly.append(f"    bne r{register[1]}, r0, {target}")
        self.release(register)

    def statement(self, node):
        kind = node[0]
        if kind == "block":
            for child in node[1]: self.statement(child)
            return
        if kind == "assign":
            slot = self.variables[node[1]]
            value = self.expression(node[2])
            if slot >= 6:
                if value[0] == "const":
                    if not -(1 << 17) <= value[1] < (1 << 17):
                        raise ToolchainError("constant does not fit ADDI immediate")
                    self.assembly.append(f"    addi r11, r0, {value[1]}")
                    self.assembly.append(f"    sw r11, {self.spill_offset(slot)}(r13)")
                else:
                    loaded=self.load(value)
                    self.assembly.append(f"    sw r{loaded[1]}, {self.spill_offset(slot)}(r13)")
                    self.release(value)
                return
            destination = 2+slot
            if value[0] == "const":
                if not -(1 << 17) <= value[1] < (1 << 17): raise ToolchainError("constant does not fit ADDI immediate")
                self.assembly.append(f"    addi r{destination}, r0, {value[1]}")
            else:
                loaded=self.load(value)
                self.assembly.append(f"    addi r{destination}, r{loaded[1]}, 0")
                self.release(value)
            return
        if kind == "if":
            then_label, end_label = self.label("then"), self.label("ifend")
            else_label = self.label("else") if node[3] is not None else end_label
            self.condition_branch(node[1], then_label)
            self.assembly.append(f"    jal r0, {else_label}")
            self.assembly.append(then_label+":")
            self.statement(node[2])
            if node[3] is not None:
                self.assembly.append(f"    jal r0, {end_label}")
                self.assembly.append(else_label+":")
                self.statement(node[3])
            self.assembly.append(end_label+":")
            return
        if kind == "while":
            loop_label, body_label, end_label = self.label("loop"), self.label("body"), self.label("wend")
            self.assembly.extend((loop_label+":",))
            self.condition_branch(node[1], body_label)
            self.assembly.append(f"    jal r0, {end_label}")
            self.assembly.append(body_label+":")
            self.statement(node[2])
            self.assembly.append(f"    jal r0, {loop_label}")
            self.assembly.append(end_label+":")
            return
        raise ToolchainError(f"unsupported C statement {kind}")

    def generate(self, statements):
        for statement in statements:
            if statement[0] == "return":
                value = self.expression(statement[1])
                if value[0] == "const":
                    if not -(1 << 17) <= value[1] < (1 << 17): raise ToolchainError("return value does not fit ADDI immediate")
                    self.assembly.append(f"    addi r1, r0, {value[1]}")
                else:
                    loaded=self.load(value)
                    self.assembly.append(f"    addi r1, r{loaded[1]}, 0")
                    self.release(value)
                if self.current_function == "main":
                    self.assembly.append("    halt")
                else:
                    for register, offset in ((8,0),(9,4),(10,8),(11,12),(15,16)):
                        self.assembly.append(f"    lw r{register}, {offset}(r13)")
                    self.assembly.append(f"    addi r13, r13, {self.frame_size}")
                    self.assembly.append("    ret")
            else:
                self.statement(statement)
        return "\n".join(self.assembly)+"\n"

    def generate_program(self):
        for function in self.functions:
            self.current_function = function["name"]
            self.variables = function["variables"]
            self.spill_count = max(0, len(self.variables)-6)
            max_outgoing = max(self.call_arities(function["statements"]), default=0)
            self.outgoing_count = max(0, max_outgoing-6)
            self.outgoing_offset = self.FRAME_HEADER_SIZE+self.spill_count*4
            self.temp_spill_base=self.outgoing_offset+self.outgoing_count*4
            self.temp_spill_capacity=2*self.expression_node_count(function["statements"])+8
            raw_frame_size = self.temp_spill_base+self.temp_spill_capacity*4
            self.frame_size = (raw_frame_size+15)&~15
            self.free_temps = [11, 10, 9, 8]
            self.temp_locations={}
            self.temp_spill_slots={}
            self.next_temp_id=0
            prefix=self.assembly
            self.assembly=[]
            self.assembly.append(function["name"]+":")
            if self.current_function == "main": self.assembly.append("_start:")
            self.assembly.append(f"    addi r1, r13, -{self.frame_size}")
            self.assembly.append("    cmplt r1, r1, r14")
            self.assembly.append("    bne r1, r0, .Lstkfail")
            self.assembly.append(f"    addi r13, r13, -{self.frame_size}")
            if self.current_function != "main":
                for register, offset in ((8,0),(9,4),(10,8),(11,12),(15,16)):
                    self.assembly.append(f"    sw r{register}, {offset}(r13)")
                for index in range(6,len(function["params"])):
                    slot = self.variables[function["params"][index]]
                    incoming_offset = self.frame_size+4*(index-6)
                    self.assembly.append(f"    lw r11, {incoming_offset}(r13)")
                    self.assembly.append(f"    sw r11, {self.spill_offset(slot)}(r13)")
            self.generate(function["statements"])
            provisional_frame_size=self.frame_size
            raw_frame_size=self.temp_spill_base+4*len(self.temp_spill_slots)
            self.frame_size=(raw_frame_size+15)&~15
            if self.frame_size>131056:
                raise ToolchainError("function frame exceeds the signed ADDI stack-adjustment limit")
            patched=[]
            for line in self.assembly:
                if line==f"    addi r13, r13, -{provisional_frame_size}":
                    line=f"    addi r13, r13, -{self.frame_size}"
                elif line==f"    addi r1, r13, -{provisional_frame_size}":
                    line=f"    addi r1, r13, -{self.frame_size}"
                elif line==f"    addi r13, r13, {provisional_frame_size}":
                    line=f"    addi r13, r13, {self.frame_size}"
                else:
                    for index in range(6,len(function["params"])):
                        old_offset=provisional_frame_size+4*(index-6)
                        if line==f"    lw r11, {old_offset}(r13)":
                            line=f"    lw r11, {self.frame_size+4*(index-6)}(r13)"
                            break
                patched.append(line)
            self.assembly=prefix+patched
        self.assembly.extend((".Lstkfail:","    ecall"))
        return "\n".join(self.assembly)+"\n"

    def spill_offset(self, slot):
        return self.FRAME_HEADER_SIZE+4*(slot-6)


def compile_c(source, isa_version=3):
    """Compile the TinyC subset; isa_version selects the emitted `.isa` directive."""
    parsed = TinyCParser(source).parse()
    assembly = TinyCCodegen(parsed["functions"], parsed["externs"], isa_version).generate_program()
    return assemble(assembly)


def dump_hobj(blob, base=HXF_BASE):
    """Return disassembly lines for an HOBJ .text section with symbols."""
    obj = parse_hobj(blob)
    text = next((s for s in obj["sections"] if s["name"] == ".text"), None)
    if text is None:
        raise ToolchainError("HOBJ has no .text section")
    text_index = next(i for i, s in enumerate(obj["sections"]) if s["name"] == ".text")
    symbols = {}
    for sym in obj["symbols"]:
        if sym["defined"] and sym["section"] == text_index:
            symbols[base + sym["value"]] = sym["name"]
    return disassemble_bytes(bytes(text["data"]), base, symbols)


def dump_hxf(blob):
    """Return disassembly lines for executable LOAD segments in an HXF image."""
    if len(blob) < HXF_HEADER or blob[:4] != b"HXF1":
        raise ToolchainError("invalid HXF magic/header")
    entry = struct.unpack_from("<I", blob, 16)[0]
    table_off = struct.unpack_from("<I", blob, 20)[0]
    seg_count = struct.unpack_from("<H", blob, 24)[0]
    seg_size = struct.unpack_from("<H", blob, 26)[0]
    if seg_size != HXF_SEGMENT or seg_count < 1:
        raise ToolchainError("unsupported HXF segment table")
    lines = [f"; HXF entry 0x{entry:08x}"]
    symbols = {entry: "_start"}
    for i in range(seg_count):
        off = table_off + i * HXF_SEGMENT
        typ, flags, file_off, vaddr, file_size, mem_size, _align, _res = struct.unpack_from(
            "<IIIIIIII", blob, off
        )
        if typ != 1:
            continue
        if (flags & 4) == 0 or file_size == 0:
            continue
        payload = blob[file_off : file_off + file_size]
        lines.append(f"; segment vaddr=0x{vaddr:08x} size={file_size}")
        lines.extend(disassemble_bytes(payload, vaddr, symbols))
    if len(lines) == 1:
        raise ToolchainError("HXF has no executable payload to dump")
    return lines


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    asm = sub.add_parser("as", help="assemble source into HOBJ (HAEUN-AS)")
    asm.add_argument("source", type=Path)
    asm.add_argument("output", type=Path)
    ld = sub.add_parser("ld", help="link HOBJ files into HXF v1 (HAEUN-LD)")
    ld.add_argument("objects", nargs="+")
    ld.add_argument("-o", "--output", required=True, type=Path)
    hfw = sub.add_parser("hfw", help="link firmware objects into HFW v1 at address 0")
    hfw.add_argument("objects", nargs="+")
    hfw.add_argument("-o", "--output", required=True, type=Path)
    cc = sub.add_parser("cc", help="compile the supported C integer subset into HOBJ")
    cc.add_argument("source", type=Path)
    cc.add_argument("output", type=Path)
    cc.add_argument("--isa", type=int, default=3, choices=(1, 2, 3, 4))
    dump = sub.add_parser("dump", help="disassemble HOBJ or HXF (HAEUN-DUMP)")
    dump.add_argument("image", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "as":
            args.output.write_bytes(assemble(args.source.read_text(encoding="utf-8")))
        elif args.command == "cc":
            args.output.write_bytes(
                compile_c(args.source.read_text(encoding="utf-8"), isa_version=args.isa)
            )
        elif args.command == "hfw":
            args.output.write_bytes(
                link_firmware([Path(name).read_bytes() for name in args.objects])
            )
        elif args.command == "ld":
            args.output.write_bytes(
                link_hobjs([Path(name).read_bytes() for name in args.objects])
            )
        elif args.command == "dump":
            blob = args.image.read_bytes()
            if blob[:4] == b"HXF1":
                text_lines = dump_hxf(blob)
            elif blob[:4] == b"HOBJ":
                text_lines = dump_hobj(blob)
            else:
                raise ToolchainError("dump expects HOBJ or HXF input")
            sys.stdout.write("\n".join(text_lines) + "\n")
        else:
            raise ToolchainError(f"unknown command {args.command}")
    except (OSError, UnicodeError, ToolchainError, struct.error, MemoryError) as exc:
        print(f"haeun-toolchain: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
