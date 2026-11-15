# HAEUN boot-information record v1 (HBI)

HBI is the little-endian, versioned handoff record supplied by the runtime to guest entry code. Version 1 is exactly 32 bytes. It is serialized field by field; native C structs are never written as the wire format.

| Offset | Size | Field | Required v1 value / behavior |
|---:|---:|---|---|
| 0 | 4 | magic | ASCII `HBI1` |
| 4 | 2 | version | `1` |
| 6 | 2 | record_size | `32` |
| 8 | 4 | flags | `0`; unknown flags reject |
| 12 | 4 | ram_size | guest RAM size in bytes |
| 16 | 4 | stack_base | inclusive lower stack bound |
| 20 | 4 | stack_top | exclusive initial stack top |
| 24 | 4 | isa_version | guest ISA version, 1..4 |
| 28 | 4 | reserved | zero |

All multi-byte fields are unsigned little-endian integers. A valid record has nonzero RAM, `stack_base < stack_top <= ram_size`, a 4 KiB aligned stack base, and a 16-byte aligned stack top. The v1 codec accepts a containing buffer larger than 32 bytes but consumes exactly the fixed record. Invalid records are rejected without partially updating the decoded output.

## Initial runtime handoff

The current VM CLI reserves the page `0x0FEFF000–0x0FEFFFFF` for boot information and places the 32-byte record at `0x0FEFF000`. This page lies above the HXF v1 user segment limit and below the initial stack reservation. At guest entry:

- `r2` points to the HBI record.
- `r3` contains its size, 32.
- `r13` is the exclusive stack top, `0x10000000` for the 256 MiB profile.
- `r14` is the inclusive stack lower bound, `0x0FF00000`.

The record carries the same stack bounds plus RAM size and ISA version. Compiler-generated code currently uses `r14` for stack checks; it does not yet parse HBI itself. The C codec validates and serializes HBI, and the VM CLI uses that codec for the handoff. A future bootloader must preserve this v1 layout or publish a new record version.
