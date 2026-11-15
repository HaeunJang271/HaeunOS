# HOBJ relocatable object format v4

HOBJ v4 is byte-for-byte structurally compatible with HOBJ v3, except `format_version=4` and the header's former reserved `u16` at offset 14 contains `guest isa_version=3`. All other fields, sections, symbols, relocation rules, and bounds constraints are unchanged; see [HOBJ v2](hobj-v2.md) for the complete base layout and [HOBJ v3](hobj-v3.md) for the version-field convention.

Assemblers emit HOBJ v4 for `.isa 3` input. Linkers accept HOBJ v1 through v4, require every input object to target the same ISA, and place ISA version 3 in the HXF v1 header. The v4 version identifies the ISA contract; it does not change relocation encoding.
