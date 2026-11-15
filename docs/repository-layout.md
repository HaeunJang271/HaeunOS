# Repository layout

This is the intended tree for HAEUN-OS. Directories marked **active** contain the current foundation. Directories marked **reserved** exist so future work has a stable home; they must not contain fake kernels, stubs that claim to boot, or host-native substitutes for HAEUN code.

| Path | Status | Role |
|---|---|---|
| `docs/` | active | Architecture, ISA versions, platform, ABI, formats, roadmap |
| `docs/architecture/` | active | CPU emulator module map (`haeun-isa.md`) |
| `include/haeun/` | active | Public C17 headers for the host VM and image codecs |
| `src/` | active | Host-side VM, HXF/HFW/HBI codecs, CLI |
| `tests/` | active | C17 and Python verification for ISA, devices, formats, toolchain |
| `tools/` | active | Assembler, linker, disassembler, ISA JSON, tiny C subset compiler |
| `tools/isa/` | active | Authoritative `haeun_isa.json` opcode metadata |
| `examples/` | active | Sample HAEUN Assembly and C subset programs |
| `firmware/` | active | HAEUN firmware sources (`boot.s`) and HFW inputs |
| `kernel/` | active | Modular foundation (`core`/`arch`/`mm`/…) plus `scheduler/` demo |
| `userland/` | active | Init, shell, libc stubs for HAEUN-32 |
| `guest/` | reserved | Extra user programs beyond `userland/` |
| `lib/` | reserved | Future shared guest-facing support code, if needed |
| `build/` | generated | Out-of-tree build products; not committed |

## Host versus guest boundary

- Code under `src/` and `include/haeun/` runs on the **host** (x86-64 Windows/Linux) and emulates the guest.
- Code under `firmware/`, `kernel/`, and `guest/` must be **HAEUN-32** sources assembled or compiled by the HAEUN toolchain and executed only by the VM.
- No path may silently replace HAEUN instruction streams with host-native object code and present that as guest execution.

## Build entry points

- `CMakeLists.txt` — primary C17 build and CTest registration.
- `Makefile` — alternate dependency-light build and test driver.
- `tools/haeun_toolchain.py` — assembler, linker, disassembler (`dump`), and C subset compiler.
- `tools/isa/haeun_isa.json` — shared opcode metadata for toolchain and generated C headers.
