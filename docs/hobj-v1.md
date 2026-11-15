# HOBJ relocatable object format v1

HOBJ v1 is the original deterministic little-endian object format. It is still accepted as a single linker input for compatibility. New assembler output uses [HOBJ v2](hobj-v2.md), which adds multiple objects, data sections, and symbol binding. All serialized fields use byte-wise little-endian encoding; host C/Python structs are never written directly.

## Header: 40 bytes

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic `HOBJ` |
| 4 | 2 | format version, exactly 1 |
| 6 | 2 | header size, exactly 40 |
| 8 | 2 | section count, exactly 1 |
| 10 | 2 | symbol count |
| 12 | 2 | relocation count |
| 14 | 2 | reserved, zero |
| 16 | 4 | section table offset, exactly 40 |
| 20 | 4 | symbol table offset |
| 24 | 4 | relocation table offset |
| 28 | 4 | section data offset |
| 32 | 4 | section data size |
| 36 | 4 | total object size, exact file length |

Tables are contiguous and ordered: one 28-byte section descriptor, `symbol_count` 24-byte symbol records, `relocation_count` 16-byte relocation records, then raw section bytes. Every offset and size must match this layout exactly; trailing bytes are invalid.

## Section descriptor: 28 bytes

`name[8]` is exactly `.text` followed by three zero bytes; then `type:u32` is 1 (PROGBITS), `flags:u32` is 5 (read and execute), `alignment:u32` is 4, `file_offset:u32` equals header data offset, `size:u32` equals header data size. The linker rejects any other section shape.

## Symbol: 24 bytes

`name[16]` is a nonempty ASCII assembler identifier, NUL terminated and zero padded; names are at most 15 bytes. `value:u32` is a byte offset within `.text`. `flags:u32` is 1 for defined, 0 for undefined. Undefined symbols have value zero. Symbols are unique and sorted by bytewise ASCII name. The initial linker accepts only defined symbols and requires `_start`.

## Relocation: 16 bytes

`offset:u32` is a four-byte-aligned instruction offset in `.text`; `symbol_index:u16` indexes the symbol table; `type:u16` is 1 (`PCREL18_WORDS`); `addend:i32` is a signed byte addend; `reserved:u32` is zero. Relocations are strictly increasing by offset, target `BEQ`, `BNE`, or `JAL`, and patch bits 17:0. The linker computes `(symbol_value + addend - (offset + 4)) / 4`; the byte delta must be divisible by four and the word displacement must fit signed 18 bits. The fixed HXF link base is `0x00100000`, which cancels from this same-section calculation.

## Initial assembler syntax

The accepted mnemonics are the full HAEUN-32 ISA v1 table. Registers are `r0` through `r15`; commas separate operands. `LW`/`SW` use `register, signed_offset(base_register)`. Symbolic BEQ/BNE/JAL targets create PCREL18 relocations; numeric targets are signed instruction-word displacements. Integers are decimal or `0x` hexadecimal. `#` and `;` begin line comments. Labels use ASCII identifiers.

Supported directives are `.byte value[, ...]`, `.word value[, ...]`, and `.align power_of_two` (1 through 4096). `.byte` emits one byte per value in signed 8-bit range; `.word` emits little-endian signed 32-bit values; `.align` emits zero bytes. Instructions must begin on four-byte boundaries. There are no macros, string literals, named sections, global visibility directives, or expressions beyond integer literals and labels.

## Link behavior and limitations

The initial linker consumes one HOBJ file, places `.text` at `0x00100000`, applies the relocations, requires aligned `_start` within text, and emits one RX HXF v1 LOAD segment with file size equal to memory size. Undefined symbols, unsupported section or relocation types, malformed bounds/reserved bytes, and out-of-range relocations reject the object. Multi-object linking and data/BSS sections are future extensions and require a new HOBJ version or a backward-compatible extension.
