# Firmware image and reset-to-entry boot flow

Status: **HFW v1 validation/staging, firmware execution, and disk-mediated HXF→HBI transfer are implemented and tested**. The foundation kernel boots through this path ([kernel-foundation.md](kernel-foundation.md)). The `haeun-vm <image.hxf>` mode remains a separate direct guest-image runner.

## HFW firmware image v1

HFW v1 contains HAEUN-32 instruction bytes for the firmware reservation at physical address zero. All integer fields are little-endian. The fixed header is 32 bytes, followed by exactly `file_size` payload bytes; trailing bytes are invalid.

| Offset | Size | Field | Constraint |
|---:|---:|---|---|
| 0 | 4 | magic | ASCII `HFW1` |
| 4 | 2 | format_version | `1` |
| 6 | 2 | header_size | `32` |
| 8 | 4 | guest ISA version | `1`..`4` |
| 12 | 4 | load address | `0` |
| 16 | 4 | reset entry offset | `0` |
| 20 | 4 | file_size | payload bytes, at least 4 and divisible by 4 |
| 24 | 4 | memory_size | at least `file_size`, at most 65536, divisible by 4 |
| 28 | 4 | reserved | zero |

Payload begins at file offset 32 and is copied to guest physical address zero. The loader zero-fills `[file_size, memory_size)`. Firmware instruction fetch is intended to be limited to the first 64 KiB reservation; the current flat-RAM VM does not enforce that boundary. HFW v1 has no checksum or signature.

## Reset and boot flow

1. `haeun-vm --firmware <image.hfw>` validates the HFW container and stages its payload into guest RAM. This operation only copies bytes; it does not interpret or execute guest instructions on the host.
2. The VM starts at PC `0`, selects the HFW-declared ISA, write-protects Boot ROM code (`0x0000–0x7FFF`), initializes board state, and begins normal HAEUN instruction execution. Initial stack registers use the board profile: `r13=0x10000000`, `r14=0x0FF00000`. Other general registers start at zero.
3. With `--disk`, firmware drives the block MMIO device, validates a constrained one-segment HXF, loads it, and constructs the HBI v1 record at `0x0FEFF000` (see [boot-disk-v1.md](boot-disk-v1.md)).
4. Firmware transfers to the HXF entry with `r2=0x0FEFF000`, `r3=32`, `r13` set to the selected stack top, and `r14` set to its inclusive lower bound.

Firmware image execution is always HAEUN-32 guest execution by the C17 VM. The existing direct-HXF CLI path remains useful for tests; it is a separate boot mode and directly supplies HBI and entry registers.

## Implementation dependencies

The C17 HFW loader validates every v1 header/payload constraint before changing RAM and returns the reset PC and ISA version. C tests cover malformed headers, size and range failures, and zero-fill. End-to-end tests run firmware from PC zero and, with `--disk`, load a multi-block HXF, build HBI, and halt in the guest entry.
