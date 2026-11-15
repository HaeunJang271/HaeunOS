# M20 Architecture Audit — Interactive Userland & System Services

Status: **read-only audit complete** (no M20 feature code in this document’s evidence baseline).  
Date: 2026-11-15. Classification: IMPLEMENTED / PARTIAL / NOT IMPLEMENTED / UNKNOWN.

## 0. Method

- Cross-checked `include/`, `src/`, `kernel/process/`, `userland/`, `tools/`, `sdk/`, `tests/`, `docs/`.
- Verified ECALL map against `include/haeun/sec.h` (`HAEUN_SYS_MAX = 26`).
- Confirmed `tools/build_app_hxf.py` packs raw HOBJ section bytes without applying PCREL18 relocations (vs `link_hobjs` / `build_process_image.py`).
- Ran existing M14–M17 probes as regression baseline for the audit session.

## 1. Summary table

| Area | Status | Notes |
|------|--------|-------|
| Kernel / ISA v1–v4 / MMU / TLB | IMPLEMENTED | `instruction_executor.c`, `mmu.c`, `tlb.c`, `trap.c` |
| Scheduler / Process / PCB / AS | IMPLEMENTED | `proc_sched.s`, `guest_pcb.c`, NPROC=4, AS slots=3 |
| HXF loader / APP extract | IMPLEMENTED | `hxf.c`, `app_env.c` (≤4 KiB @ `0x00108000`) |
| ECALL mux / sec gate | IMPLEMENTED | `ecall_mux.c`, `sec_dispatch.c`, numbers 0–26 |
| Framebuffer + compositor | PARTIAL | 64×32 host FB; `gfx_comp.c`; no dirty regions |
| Window manager + ownership | PARTIAL | `owner_pid` + `deny_foreign`; chrome is host-only |
| Desktop chrome / strip | PARTIAL | `gfx_chrome.c` / `gfx_desk.c`; limited guest control |
| Glyph / host text helpers | PARTIAL | `gfx_font.c`, `gfx_ui.c` — **no guest WIN_DRAW_TEXT** |
| Keyboard MMIO FIFO | IMPLEMENTED | Shared `vm->kbd_rx[64]` destructive pop |
| Mouse | PARTIAL | Single sample overwrite; no queue |
| Per-process event queue | NOT IMPLEMENTED | No pid-tagged queues |
| Focus → PID input routing | NOT IMPLEMENTED | `focus_id` only |
| HFS + FD ownership | PARTIAL | Mode bits only; global 4 FDs with `owner_pid` |
| Per-process memory quota | NOT IMPLEMENTED | Fixed slot sizes + global caps |
| IPC | NOT IMPLEMENTED | Reusable patterns only (§8) |
| System services framework | NOT IMPLEMENTED | Hybrid host+guest model recommended (§9) |
| M19 SDK packaging | PARTIAL | CLI/docs OK; app link bug blocks real apps (§1.1) |
| Host networking | IMPLEMENTED | Socket ECALL 11–17; ownership checks incomplete |
| Release / Make | IMPLEMENTED | `make release` / `make sdk`; CMake is subset |

### 1.1 P0 findings (must fix before M20 userland)

1. **`build_app_hxf` does not apply relocations** — `jal`/`beq` immediates stay 0; `.word` literal pools after `jal rX,label` execute as code → user faults. Process images via `build_process_image.py` are fine.
2. **User process-v1 has no U-mapped MMIO** — apps cannot poll KBD/UART MMIO; input must be ECALL-delivered.
3. **m14/m19 tests assert halt/markers only** — they do not fail on `[haeun-fault]` from broken apps.

## 2. Mandatory audit answers

| # | Question | Answer |
|---|----------|--------|
| 1 | Per-process event queue? | **NOT IMPLEMENTED** |
| 2 | Keyboard FIFO ownership/consume | Device-owned `haeun_vm.kbd_rx`; any MMIO reader pops; process apps cannot map it |
| 3 | Window ID ↔ PID | `wm.windows[i].owner_pid` set on CREATE; enforced on destroy/fill/pixel/raise |
| 4 | Input focus handling | Global `wm.focus_id` only; keys always go to shared FIFO |
| 5 | FB + compositor | PA `0x10000` 64×32 RGBA; full clear + z-order blit ≤8 windows |
| 6 | HFS permissions | `(mode & 0444/0222)`; no uid/gid compare; FD gated by `owner_pid` |
| 7 | Per-process memory limits | **NOT IMPLEMENTED** (4 KiB code/stack slots; global window/fd/sock/app caps) |
| 8 | ECALL numbers | See §3 |
| 9 | IPC-reusable pieces | mux/sec, `current_pid`, `reap_pid`, ownership patterns, guest VA copy, SLEEP/YIELD |
| 10 | Service execution model | **Hybrid recommended**: host policy queues + guest UI processes (§9) |

## 3. ECALL map (`r1` = number)

| # | Name | Handler |
|---|------|---------|
| 0 | EXIT | mux reap + guest |
| 1–4 | WRITE/READ/OPEN/CLOSE | host I/O assist / guest console |
| 5–10 | FORK/EXEC/WAIT/SLEEP/GETPID/YIELD | guest kernel |
| 11–17 | SOCKET…SO_CLOSE | `net_sock_ecall.c` |
| 18–23 | WIN_* / GFX_COMPOSE | `gfx_ecall.c` |
| 24–26 | APP_COUNT/NAME/SPAWN | `app_ecall.c` |
| ≥27 | unused | sec deny (`HAEUN_SYS_MAX=26`) |

**M20 allocation plan (design only; not yet implemented):** 27+ for EVENT_POLL, WIN_DRAW_TEXT, IPC_*, SERVICE_* — must update `sec.h`, `sec_core.c`, `sec_dispatch.c`, `ecall_mux.c`, `libapp.s`, SDK docs/tests together.

## 4. Graphics / desktop detail

- Caps: `HAEUN_GFX_MAX_WINDOWS` (8), chrome title/buttons host-side (`gfx_chrome.c`).
- `WIN_CREATE` accepts guest-chosen backing **PA** (`r6`) — no allocator; ownership of foreign PA is a known risk.
- Host text: `haeun_gfx_draw_text_win` exists; **not** an ECALL.
- Korean glyphs: **NOT IMPLEMENTED** (bitmap font ASCII-oriented).

## 5. Input detail

- Producers: Win32 viewer (`feed_key` / mouse), `vm_input_host` script queue.
- Consumers: KBD MMIO pop; chrome stores `last_key` but does not demux to pid queues.
- Mouse IRQ/KBD IRQ bits exist; process kernel enables timer IRQ only → poll/ECALL path required.

## 6. HFS / FD

- In-memory 16 KiB HFS on `--apps` / `--app` attach; separate from `--disk`.
- `HAEUN_PROC_NFD=4` **global** table with per-fd `owner_pid`.
- Path OPEN: root basename only; READ/WRITE = 1 byte per ECALL.

## 7. Memory map (process-v1, abbreviated PA)

| Range | Use |
|-------|-----|
| `0xA000+` | PCB bank |
| `0x10000` | FB (8 KiB @ 64×32) |
| `0x18000~` | Window backing convention |
| `0x100000` | Kernel text |
| `0x108000+` | User code slots (4 KiB) |
| stacks | `0x113000+` / kstacks `0x124000+` |

Free low RAM before kernel text ≈ 960 KiB — FB upscale must stay inside this and avoid NIC DMA (`0xB000`/`0xC000`).

## 8. IPC reuse

| Piece | Role |
|-------|------|
| `ecall_mux` / sec | New number ranges |
| `haeun_guest_pcb_current_pid` | Caller identity |
| `reap_pid` | EXIT/fault cleanup hook |
| `deny_foreign` / `fd_owner_ok` | Ownership pattern |
| `copy_guest_path` / byte loads | Safe payload copy |
| SLEEP/YIELD/WAIT | Blocking substitutes |

Missing: message queues, named endpoints, shm mappings.

## 9. Recommended service model (Hybrid)

1. **Host modules**: input demux, window events, IPC queues (non-blocking assists).
2. **Guest processes**: launcher, file manager, demos (PCB slots).
3. Blocking receive via YIELD/SLEEP rewrite or new guest BLOCKED state (design Phase 2).

Do **not** invent a package manager; reuse `APP_COUNT/NAME/SPAWN` + `--app` registration.

## 10. M20 gap list

| Feature | Gap |
|---------|-----|
| Per-process input | Host pid queues + focus routing + ECALL poll/wait |
| IPC | Endpoints, send/recv, limits, reap |
| GUI text | WIN_DRAW_TEXT ECALL; document ASCII-only; consider FB size |
| Window events | FOCUS/BLUR/KEY/MOUSE/CLOSE (+ RESIZE if chrome supports) |
| Launcher | Fix app link; GUI over APP_* |
| Services | Registry in host or dedicated guest; no clash with PCB manager |
| File manager | readdir (if missing), text UI, HFS ops already partial |
| Resource reclaim | Extend `reap_pid` for events/IPC |

## 11. Regression baseline (audit session)

| Suite | Result |
|-------|--------|
| M14 process apps | PASS (halt/markers; does not catch app link faults) |
| M15 isolation | PASS |
| M16/M17 | PASS (prior session / probes) |
| M19 SDK | PASS under current weak assertions |

## 12. Design constraints for implementation phases

- Preserve MMIO ABI; do not require user MMIO for input.
- Do not weaken M15 ownership or M17 fault→ZOMBIE.
- Keep M19 `haeun` CLI compatible; fix toolchain under the hood.
- Stage M20 artifacts separately from overwriting Release 1.0.0 claims until verified (`docs/m20-completion-report.md`).
