# HOBJ v5 ISA v4 object format

HOBJ v5 is structurally compatible with HOBJ v3/v4, except `format_version=5` and the header `isa_version` field at offset 14 is `4`. CSR/`ERET` instructions require ISA 4 objects. See [HOBJ v2](hobj-v2.md) for the base layout and [isa-v4.md](isa-v4.md) for the instruction contract.
