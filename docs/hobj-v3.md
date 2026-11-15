# HOBJ format v3: ISA v2 objects

HOBJ v3 uses the exact 40-byte header, section descriptors, symbols, relocation records, and payload rules from [HOBJ v2](hobj-v2.md), with two header versioning changes:

- `format_version:u16` at offset 4 is 3.
- The former reserved `u16` at offset 14 contains guest `isa_version:u16`, exactly 2.

All other offsets, section rules, local/global symbol encoding, and relocation semantics are unchanged. The assembler selects HOBJ v3 only after a source-level `.isa 2` directive. Without it, assembly emits HOBJ v2. The linker refuses to combine objects with different ISA versions and writes the matching guest ISA value into the HXF header. This preserves the v1 opcode contract while permitting the v2 JALR opcode. HOBJ v4 uses the same header field to identify ISA v3; see [HOBJ v4](hobj-v4.md).
