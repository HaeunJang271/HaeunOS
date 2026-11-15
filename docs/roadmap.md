# Toolchain, verification and roadmap

## Compiler and assembler roadmap

1. Freeze instruction encoding and initial call/return rules with executable tests.
2. Define versioned object/executable formats field-by-field before serializers and parsers.
3. Add an assembler/linker with deterministic diagnostics and checked relocations.
4. Add a C17 cross-compiler backend (initially LLVM target or a deliberately small documented compiler) only after ABI constraints are validated. Keep compiler-generated guest code distinguishable from host VM binaries.
5. Add reproducible toolchain builds and sample guest programs that run under the VM.

The assembler, HOBJ linker, HXF writer, disassembler (`dump`), shared ISA JSON, and tiny C17 subset compiler now exist under `tools/` (see [toolchain-v1.md](toolchain-v1.md)). The compiler handles multiple integer functions, up to 64 arguments (six in registers, remaining arguments on the stack), 16-byte-aligned dynamic frames, stack-homed parameters/locals beyond six slots, expression-temporary spilling, generated stack-bound checks, calls/returns, recursion, integer/bitwise expressions, equality and signed relational comparisons, `if`/`else`, and `while`. HBI v1 is specified, encoded/decoded, and supplied by the direct-image VM CLI. HFW v1 validation/staging, reset-to-entry firmware execution, and disk-mediated constrained HXF→HBI transfer are implemented; a privileged kernel is not. General C compilation, macros, absolute data relocations, and VM-enforced stack regions are not implemented.

## Verification standards

- Every assigned opcode has encoding/decode, success, boundary, and fault tests before its semantics are declared implemented.
- Binary formats use byte-wise encoding/decoding, fixed widths, explicit endianness, bounds checks, overflow-safe range validation, reserved-field rejection, and malformed-input tests.
- VM checks include instruction-by-instruction reference vectors, deterministic replay, and differential tests against a simple reference model where practical.
- Memory safety: no unaligned host typed-pointer dereference, no signed overflow, no unchecked host allocation size, no guest address used as host pointer, and sanitizer runs on supported hosts.
- OS validation layers: unit tests, VM integration tests, boot-image tests, fault injection, scheduler stress, and eventually hardware-independent conformance suite.
- A feature is reported implemented only when its tests/build pass. A written board contract is not a device implementation.

## Initial milestone status

### Implemented in this repository

- C17 VM instruction execution for all listed ISA v1 operations and v2 JALR, with version-gated decoding and defined fault-stop reporting.
- Fixed little-endian decoding/encoding helpers and caller-owned RAM interface.
- Per-opcode tests, signed-immediate and address-boundary cases, malformed encoding checks, and selected fault tests.
- HXF v1 byte-wise image validation and flat-RAM loading, with no RAM mutation on validation failure.
- Deterministic HXF v1 writer with caller-sized buffers and a no-partial-write buffer-too-small path.
- HXF-to-VM integration test that loads and executes a guest HALT instruction.
- CLI execution of HXF images with fixed 256 MiB guest RAM and a bounded instruction budget.
- HAEUN Assembly to HOBJ v2 to HXF v1 to VM integration test.
- Multi-object HOBJ v2 linking with external global resolution and RX/RW output segments.
- Versioned ISA v2/HOBJ v3/HXF execution, including JALR, RET, v1 rejection, and mixed-ISA link rejection.
- Versioned ISA v3/HOBJ v4/HXF execution, including all six CMP conditions, reserved encoding rejection, signed compare guest tests, and v1/v2 opcode compatibility.
- Tested stack-passed arguments and stack-homed parameter/local slots in the C subset, including an eight-argument end-to-end guest call.
- Tested expression-temporary spilling and reloads in a deeply nested guest expression.
- Tested lower-bound checks for recursive frames and outgoing stack arguments; overflow stops at ECALL before crossing the configured stack limit.
- HBI v1 fixed-layout codec, malformed-record checks, and guest-visible VM CLI handoff are tested.
- HFW v1 validation and bounded RAM staging are tested, along with HAEUN firmware execution from reset PC zero under the declared ISA.
- UART MMIO address decoding, DATA/STATUS/CONTROL semantics, RX FIFO capacity/wraparound, TX callback, and faults for unimplemented apertures are tested.
- Tiny C subset compilation through HOBJ/HXF with tested return values, if/else paths, and a loop in guest execution.
- Architecture, ISA v1, intended board register contracts, kernel/ABI, HXF, boot, toolchain, and verification documents.
- CMake and Make build definitions.
- GCC `-fanalyzer` static analysis passed for the VM, loader, and CLI sources.

- UART and block-device MMIO are tested (attach, aligned DMA read/write, atomic invalid-range/error STATUS).

### Known limitations

- VM implements RAM, board MMIO, ISA v4 privilege/CSR/trap/interrupt/`SFENCE` delivery, MMU v1 walks, and a 16-entry TLB (`ptbr` or `SFENCE` flush). The guest kernel has identity maps, user mode, cooperative yield, and timer-preemptive two-task scheduling; a general-purpose C compiler does not.
- HXF loader still uses a flat RAM profile at load time; runtime permissions come only from page tables after `ptbr.MODE=1`.
- HXF v1 has no checksum/signature.
- Assembler/HOBJ v5 support CSR/`ERET` for ISA 4.
- Calling convention is provisional. Firmware disk boot and the direct-image CLI path remain available for tests.
- AddressSanitizer/UBSan could not link on the current MinGW installation because `libasan` and `libubsan` are unavailable; sanitizer runs remain outstanding.

## Next milestone

Next: optional additional syscalls (e.g. `SYS_GETC`) and wiring deferred `syscall`/`process`/`vfs` modules into the foundation path. CPU emulator modules and per-opcode matrix tests are in place; see [architecture/haeun-isa.md](architecture/haeun-isa.md). The direct-image CLI mode remains separate. The modular kernel foundation boots via firmware ([kernel-foundation.md](kernel-foundation.md)).
