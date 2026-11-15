# M20.0.1 — Build Integrity & Runtime Validation Recovery

Date: 2026-11-15  
Scope: fix app HXF relocation + strengthen M14/M19 runtime tests.  
**No M20.1 feature work** in this milestone.

## Verdict

| Acceptance criterion | Status |
|---|---|
| SDK app builds apply relocation | **IMPLEMENTED** |
| Unresolved relocations rejected | **IMPLEMENTED** |
| Bad branch targets detected | **IMPLEMENTED** |
| M19 SDK examples actually execute | **IMPLEMENTED** |
| Faulting apps fail tests | **IMPLEMENTED** |
| Normal vs abnormal exit distinguished (halt + no `[haeun-fault]`) | **IMPLEMENTED** |
| M14/M15/M17/M18-style regression | **IMPLEMENTED** (M15/M16/M17 probes + M7–M13 process suites; no separate `m18_*` harness) |
| M19 independent developer path | **IMPLEMENTED** |

## Phase 1 — Investigation (reproduced)

Assembler leaves PCREL18 immediates at **0** and records HOBJ relocs.  
`link_hobjs` / `build_process_image.py` apply them; the audit gap was packing raw `.text` for app HXF.

Repro (hello + libapp):

```text
before off=0x4 sym=app_putc imm=0
after  off=0x4 -> 0xb0 sym=app_putc imm=42
```

Calculator pool idiom after fix: `jal r6, +1` then `.word` then `lw` — literal is skipped, not executed.

Additional blockers found while reproducing:

1. HOBJ symbol names >15 chars (`app_svc_register`, `app_win_draw_text`) → assemble fail for any libapp link.
2. `app_putc` used UART MMIO (`0x10002000`); process PTEs map UART as **U=0** → user page fault even with correct reloc.
3. `app_calc.s` probed KBD MMIO (`0x10005000`) — same U-mapping gap.

## Phase 2 — Fix

### Shared linker relocation (`tools/haeun_toolchain.py`)

Extracted the real PCREL18 path used by `link_hobjs` / firmware:

- `place_hobj_sections`
- `resolve_symbol_addresses`
- `apply_pcrel18_relocations`
- `link_relocated_sections`

Semantics unchanged: unresolved / non-`.text` / misaligned / out-of-range / non-BEQ|BNE|JAL → reject.

### App HXF builder (`tools/build_app_hxf.py`)

- Assembles sources, then **`link_relocated_sections`** (does not ship raw HOBJ bytes).
- Requires `user_init` at `.text+0`.
- Verifies every branch/JAL target lands inside the code image.
- Keeps HXF v1, entry/load `0x00108000`, ≤4 KiB.

### Runtime-safe lib / apps

- `app_putc` → console `WRITE` ECALL (fd=1), not UART MMIO.
- Renamed HOBJ-illegal symbols: `app_draw_text`, `app_svc_reg`.
- Removed KBD MMIO probe from `userland/app_calc.s`.

## Phase 3–5 — Artifact & SDK validation

| Example | HXF | Disasm/reloc | VM run | Fault | Halt |
|---|---|---|---|---|---|
| hello | 436 B | OK | spawn + halt | no | yes (`r1=0x1`) |
| calculator | 564 B | pool `jal +1` OK | spawn + halt | no | yes |
| file_viewer | 636 B | OK | spawn + halt | no | yes |
| drawing | 564 B | OK | spawn + halt | no | yes |
| procinfo | 568 B | OK | spawn + halt | no | yes |

Sample log:

```text
apps: registered sdk_hello (436 bytes)
app-env: spawn sdk_hello pid=2 slot=1
guest halted after 23688 instructions; r1=0x00000001
```

**API limits (documented, not “PASS” as full product features):**

- User apps must not rely on KBD/UART MMIO (no U-map); use ECALL.
- M20 stubs in `libapp.s` (event/IPC/svc/text) exist in-tree; this milestone only made them **buildable** (≤15-char names). Full M20.1 behavior is out of scope.

## Phase 4 — Test repairs

| File | Change |
|---|---|
| `tests/m14_process_apps_tests.py` | HXF/reloc checks; fail on `[haeun-fault]` |
| `tests/m19_sdk_tests.py` | Build+verify+**run all 5 examples**; fault fail; independent SDK path |
| `tests/m20_build_integrity_tests.py` | **new** — pool reloc, unresolved reject, raw-imm≠linked |

## Phase 6 — Regression (this session)

| Suite | Result |
|---|---|
| toolchain_tests | PASS (33) |
| m20_build_integrity | PASS |
| m14 process apps | PASS |
| m15 isolation (C probe) | PASS |
| m16 reliability / repeat | PASS |
| m17 fault (C + process) | PASS |
| m19 SDK | PASS |
| m20 process | PASS |
| m7 GUI / m8 GUI / m9 net / m10 input / m11 viewer / m12 desktop / m13 net | PASS |

## Phase 7 — Deferred audit items (not fixed here)

| Issue | Repro | Impact | Suggested fix |
|---|---|---|---|
| Process MMIO U-bit | UART/KBD PTE `0x…07` (no U) | User MMIO → page fault | Keep ECALL I/O; optional future U-map only if policy allows |
| `WIN_CREATE` raw PA | Guest supplies `r6` PA | Foreign PA use | Host allocator + ownership check |
| Socket owner validation | `set_owner` on SOCKET only; bind/send/close unchecked | Cross-pid socket use | Check `owner_pid` on all sock ECALLs |
| PCB3 / fork AS aliasing | NPROC=4 vs 3 AS slots | 4th PCB may share AS | Cap runnable PCBs to slots or add AS |
| Scheduler idle | `pick_next` → none → halt | No idle loop | Idle PCB / WFI-style wait |
| `HAEUN_APP_MAX` docs | Code `MAX=4` register, `SLOTS=3` run | Doc confusion | Align docs: register ≤4, concurrent slots 1–2 |

## Modified files

- `tools/haeun_toolchain.py`
- `tools/build_app_hxf.py`
- `userland/libapp.s`, `userland/app_calc.s`
- `userland/app_m20_text.s`, `app_m20_files.s`, `app_m20_ipc.s` (symbol rename only)
- `tests/m14_process_apps_tests.py`, `tests/m19_sdk_tests.py`
- `tests/m20_build_integrity_tests.py` (new)
- `Makefile`, `sdk/docs/05-userland-api.md`, `sdk/templates/basic/src/app.s`

## Feature status board

| Item | Status |
|---|---|
| App HXF PCREL18 via shared linker | IMPLEMENTED |
| Unresolved / OOR reloc reject | IMPLEMENTED |
| Branch target image bounds check | IMPLEMENTED |
| SDK example real execution | IMPLEMENTED |
| Fault → test FAIL | IMPLEMENTED |
| Halt vs fault distinction | IMPLEMENTED |
| M20.1 interactive userland | NOT IMPLEMENTED (deferred) |
| Per-process input / IPC productization | NOT IMPLEMENTED (deferred) |
