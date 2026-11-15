# M20.0.2 — Kernel Resource Ownership & Address-Space Integrity

Date: 2026-11-15  
Scope: harden WIN_CREATE PA checks, socket ownership, PCB/AS consistency, capacity docs.  
**No M20.1 input/event feature work.**

## Verdict

| Acceptance | Status |
|---|---|
| raw PA validation tested | **IMPLEMENTED** |
| Socket ECALL ownership table confirmed | **IMPLEMENTED** |
| PCB/AS invariants documented + fork capped | **IMPLEMENTED** |
| AS reuse clears prior stack/code | **IMPLEMENTED** (spawn scrub) |
| M7–M20.0.1 regression | **PASSED** (this session) |
| New security tests verify deny + reclaim | **IMPLEMENTED** |
| Idle task | **PARTIAL** (wake_all retry; no dedicated idle PCB) |

## Phase 1 — Baseline (code-verified)

| Area | Files | Finding | Fix? |
|---|---|---|---|
| WIN_CREATE PA | `gfx_ecall.c`, `gfx_wm.c` | Guest PA only RAM-bounded; could target PCB/PT/kernel | Yes |
| Socket ownership | `net_sock_ecall.c` | `owner_pid` set on SOCKET only; ops unchecked; ACCEPT unowned | Yes |
| PCB3 alias | `proc_fork.s` | `fk_scan` to 4 → slot 3 maps AS2 | Yes |
| `HAEUN_APP_MAX` | `app_env.h`=4 vs docs/CLI “8” | Doc/CLI mismatch | Yes (docs+CLI) |
| Idle | `proc_sched.s`, `proc_sys.s` | No READY → `halt`; SLEEP may be stranded | Partial |

## Phase 2 — WIN_CREATE (ABI preserved)

Guest still passes PA in `r6`. Added checks:

1. 4-byte alignment + RAM bounds (`gfx_wm.c`)
2. No overlap with other window backings
3. No overlap with primary FB when `gfx` known (ECALL path)
4. Process apps (`pid != 0`): PA must lie in `[HAEUN_WIN_POOL_LO, HAEUN_WIN_POOL_HI)` = `[0x18000, 0x100000)`
5. `owned_by` is exact pid match; `owner_pid==0` is host-only

Legacy unit tests without PCB (`pid==0`) keep low-RAM bases.

## Phase 3 — Socket ownership table

| ECALL | Owner check | Notes |
|---|---|---|
| SOCKET | N/A (sets owner) | `set_owner` when `pid!=0` |
| BIND | **yes** | `haeun_sock_owned_by` |
| SENDTO | **yes** | |
| RECVFROM | **yes** | meta copy failure no longer silent |
| LISTEN | **yes** | |
| ACCEPT | **yes** on listen fd; **sets owner** on new fd | |
| SO_CLOSE | **yes** | |
| EXIT / fault reap | **yes** | existing `close_by_owner` |

`pid==0` (no PCB) may only use unowned sockets → legacy sock ECALL tests keep working.

## Phase 4 — PCB / AS invariants

1. **AS slots** = `HAEUN_APP_SLOTS` = 3 (indices 0..2).  
2. **PCB table** = `HAEUN_GUEST_PCB_NPROC` = 4 (RAM layout unchanged).  
3. **FORK** only scans slots `0..2` (`proc_fork.s`); PCB3 aliasing refused (`-1`).  
4. **SPAWN** wipes code **and** user stack page before load; reap still scrubs PTE/code/stack + TLB flush.  
5. Active process PCB must use `ptbr`/`stacks` for its slot; fourth PCB cannot be forked.

## Phase 5 — Capacity

| Constant | Value | Role |
|---|---|---|
| `HAEUN_APP_MAX` | 4 | Registry / CLI `--app` cap |
| `HAEUN_APP_SLOTS` | 3 | Concurrent AS (launcher + 2 apps) |
| `HAEUN_GUEST_PCB_NPROC` | 4 | Guest PCB bank size |
| `HAEUN_GUEST_PCB_AS_SLOTS` | 3 | FORK/AS limit |

Docs updated: `sdk/docs/10-limitations.md`, `docs/apps-v1.md`, `docs/sdk-m19-report.md`, `docs/security-isolation-v1.md`.  
`main.c` uses `HAEUN_APP_MAX` (not 8).

## Phase 6 — Idle

**Not** a full idle PCB. On `EXIT` / fault with no READY:

1. `wake_all` (expire SLEEP)
2. `pick_next` again
3. else `halt`

**Remaining limits:** WAIT-only deadlock still resumes incorrectly; future-deadline SLEEP with no READY peer can still halt the VM. Documented intentional v1 limit.

## Phase 7 — New tests

`tests/m20_resource_integrity_tests.c` → `build/m20-resource-integrity-tests.exe`

- unaligned / pool / PCB PA reject  
- foreign window FILL deny  
- foreign socket BIND/CLOSE deny + bad FD  
- overlap reject + destroy reuse  
- capacity constant checks  

Also updated `m15_isolation_tests.c`, `m16_reliability_tests.c` for pool/overlap semantics.

## Phase 8 — Commands & results (this session)

```text
.\build\m20-resource-integrity-tests.exe     → ok
.\build\m15-isolation-tests.exe              → ok
.\build\m16-reliability-tests.exe            → ok
.\build\m17-fault-tests.exe                  → ok
python tests/m14_process_apps_tests.py ...   → passed
python tests/m19_sdk_tests.py ...            → passed
python tests/m20_build_integrity_tests.py ...→ passed
python tests/m9_process_net_tests.py ...     → passed
python tests/m13_process_net_tests.py ...    → passed
python tests/m7/m8/m10/m11/m12 ...           → passed
python tests/toolchain_tests.py ...          → OK (33)
```

External real-NIC soak was **not** re-run beyond M13 process harness → not claimed as separate PASS.

## Modified files (primary)

- `include/haeun/gfx_wm.h`, `src/gfx_wm.c`, `src/gfx_ecall.c`
- `include/haeun/net_sock.h`, `src/net_sock.c`, `src/net_sock_ecall.c`
- `include/haeun/guest_pcb.h`, `src/guest_pcb.c`, `src/app_env.c`, `src/main.c`
- `kernel/process/proc_fork.s`, `proc_sys.s`
- `tests/m20_resource_integrity_tests.c` (new), `m15_*`, `m16_*`
- `Makefile`, capacity/isolation docs

## Intentional remaining limits

- Guest still chooses PA inside the pool (no host allocator yet)
- `GFX_COMPOSE` remains global
- Full idle PCB / WFI loop not implemented
- NIC UDP/TCP host state not fully cleared on sock close (pre-existing)
- M20.1 event/input system not started
