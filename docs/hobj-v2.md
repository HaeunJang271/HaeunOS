# HOBJ relocatable object format v2

HOBJ v2 extends v1 with multiple input objects, `.text`/`.data` sections, and local/global symbol binding. Every integer field is little-endian. The 40-byte header layout is unchanged from [HOBJ v1](hobj-v1.md), except `format_version=2`; the section count is 1..16. Tables remain contiguous and ordered: section descriptors, symbols, relocations, then section payload bytes. Header offsets, data size, and file size must exactly describe the file with no trailing bytes.

## Sections

Each 28-byte section descriptor is `name[8], type:u32, flags:u32, alignment:u32, file_offset:u32, size:u32`. Names are `.text` or `.data`, NUL padded. Type is 1 (PROGBITS). `.text` flags are 5 (read/execute); `.data` flags are 3 (read/write). Alignment is a power of two from 1 through 4096. Sections have nonzero size, appear at contiguous nonoverlapping payload ranges in descriptor order, and the supported names occur at most once. The initial assembler emits `.text` first, then `.data` when present.

## Symbols

Each 24-byte symbol record contains `name[16]`, `section_binding:u32`, `value:u32`. Names are unique, sorted ASCII identifiers, NUL terminated and zero padded, at most 15 bytes. For a defined local symbol, `section_binding` is the zero-based section index. For a defined global symbol, bit 31 is set and the low 16 bits hold the section index; bits 16..30 are zero. `0xFFFFFFFF` means an undefined global symbol, which requires value zero. `value` is the byte offset within the named section and may equal its size for non-entry end labels.

`.global name` exports a defined label or declares an external symbol. Other labels remain local to their object. Cross-object references must be declared `.global`; the linker resolves them against exactly one global definition. Duplicate global definitions and unresolved globals reject the link.

## Relocations

Each 16-byte relocation is `payload_offset:u32, symbol_index:u16, type:u16, addend:i32, reserved:u32`. Payload offset is the absolute file offset of the instruction in the concatenated section data area, and records are strictly increasing by that offset. Type 1 is `PCREL18_WORDS`; reserved must be zero. The location must be a four-byte-aligned instruction in `.text`, and the symbol must resolve to `.text`. Linker formula is `(target_address + addend - (place_address + 4)) / 4`; the byte delta must be divisible by four and the result fit signed 18 bits. Supported relocation sites are BEQ, BNE, and JAL.

## Assembly and link placement

Source defaults to `.text`; `.text` and `.data` switch sections. `.global`, `.byte`, `.word`, and `.align` are supported. Instructions are valid only in `.text`; `.byte` and `.word` are valid in either section. `.align` emits zero bytes and raises the section alignment. The assembler accepts ISA v1 mnemonics by default and gates ISA v2/v3 extensions behind `.isa 2` or `.isa 3`.

The linker accepts multiple HOBJ v1/v2/v3/v4 files, places combined text at `0x00100000`, concatenating inputs in command-line order and honoring section alignment. Combined data begins at the next 4 KiB boundary after text. Output HXF has one RX text segment and, when present, one RW data segment. The initial loader's 256 MiB flat-RAM limit applies. Exactly one `_start` definition is required in text. HOBJ v1 has no binding field, so it is intended as a single input; multi-object links should use v2 or later. All objects in one link must target the same ISA version.

The current ISA has no absolute-address load-immediate sequence, so this version does not define an absolute data-address relocation. Data sections can be laid out and loaded, while code can only branch/jump to text symbols.
