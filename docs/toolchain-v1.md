# HAEUN toolchain v1

Status: **assembler, linker, object/executable formats, and disassembler are implemented and tested**. This is not a full compiler toolchain.

## Component names

| Name in requests | In this repository |
|---|---|
| HAEUN-AS | `python tools/haeun_toolchain.py as` |
| HAEUN-LD | `python tools/haeun_toolchain.py ld` |
| HAEUN-OBJ | HOBJ v2–v5 (`docs/hobj-*.md`) |
| HAEUN-HXE | **HXF v1** (`docs/software-abi.md`) — there is no separate `HXE` magic |
| HAEUN-DUMP | `python tools/haeun_toolchain.py dump` |

## Shared ISA metadata

Authoritative instruction table: `tools/isa/haeun_isa.json`.

- Assembler/disassembler load opcode tables from that JSON (`tools/haeun_isa.py`).
- Emulator C opcodes are generated into `include/haeun/isa_opcodes.h` by `tools/gen_isa_opcodes.py`.
- `make test` / toolchain tests run `gen_isa_opcodes.py --check` so the header cannot drift silently.

Do not hand-edit duplicate opcode maps in unrelated files.

## Round-trip

Verified path:

1. Assembly source → HOBJ (`as`)
2. HOBJ → HXF (`ld`)
3. HOBJ/HXF → text listing (`dump`)

Tests live in `tests/toolchain_tests.py` (`test_round_trip_assembly_object_executable_disassembly`).

## Limits

- Tiny C subset compiler exists (`cc`) but is not claimed as general C compatibility.
- Absolute data relocations and rich debug info are out of scope for v1.
- Disassembly of data-only segments is skipped; only executable LOAD / `.text` is listed.
