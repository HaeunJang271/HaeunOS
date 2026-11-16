# HAEUN-OS M20 Final Acceptance Report

Date: 2026-11-15  
Scope: M20.0.1–M20.8 automated acceptance after clean rebuild.  
**Source code was not modified for this report.**

## Verdict

**AUTOMATED ACCEPTANCE PASSED**

- `RELEASE ACCEPTED`: **No** (manual GUI / physical input not completed).  
- Manual GUI checklist: **PENDING** (see §6).  
- This verdict means only: the commanded automated regression completed with exit 0.

## Evidence source (this session)

| Item | Value |
|---|---|
| Commands | `make clean` → `make all` → `make test` |
| Host log | Cursor terminal `6.txt` |
| `last_exit_code` | **0** |
| Prior session `make test` | Also exit 0 (`build/m20_8_regression.log`); cited only as history — **this report treats the terminal `6.txt` run as the acceptance run** |

Normal guest `halt` in process smokes is **not** treated as feature proof by itself; process tests assert markers (`spawn`, no `[haeun-fault]`, expected strings) in addition to halt.

---

## 1. Automated regression summary

### 1.1 Full `make test`

- Result: **PASS** (exit 0).  
- Failures recorded this run: **0**.  
- Optional external DNS case inside `m13-net-backend-tests`: **SKIPPED** when `HAEUN_NET_EXTERNAL` is unset (`dns_external: skipped`). The m13 backend binary itself still returned success.

Exact per-binary pass lines early in the scrollback are truncated in the captured terminal buffer; the run completed through the final M20 process suite with exit 0. M20-specific lines below were observed in the same log.

### 1.2 M20-specific automated suite (observed PASS)

| # | Command / binary | Result |
|---|---|---|
| 1 | `python tests/m20_build_integrity_tests.py` | PASS |
| 2 | `./build/m20-unit-tests.exe` | PASS |
| 3 | `./build/m20-resource-integrity-tests.exe` | PASS |
| 4 | `./build/m20-input-tests.exe` | PASS |
| 5 | `./build/m20-ipc-tests.exe` | PASS |
| 6 | `./build/m20-text-tests.exe` | PASS |
| 7 | `./build/m20-window-tests.exe` | PASS |
| 8 | `./build/m20-launcher-tests.exe` | PASS |
| 9 | `./build/m20-service-tests.exe` | PASS |
| 10 | `./build/m20-file-tests.exe` | PASS |
| 11 | `./build/m20-integration-tests.exe` | PASS |
| 12 | `python tests/m20_process_tests.py` | PASS |
| 13 | `python tests/m20_input_process_tests.py` | PASS |
| 14 | `python tests/m20_ipc_process_tests.py` | PASS |
| 15 | `python tests/m20_text_process_tests.py` | PASS |
| 16 | `python tests/m20_window_process_tests.py` | PASS |
| 17 | `python tests/m20_launcher_process_tests.py` | PASS |
| 18 | `python tests/m20_file_process_tests.py` | PASS |
| 19 | `python tests/m20_integration_process_tests.py` | PASS |

**M20 automated count:** 19 executed, **19 PASS**, **0 FAIL**, **0 SKIP** (within this M20 list).

**Repo-wide optional SKIP (not M20-numbered):** external DNS (`HAEUN_NET_EXTERNAL` unset) — **1 SKIPPED case** inside an otherwise passing m13 suite.

---

## 2. Documented limits (do not over-claim)

1. **AS slots = 3** (`HAEUN_APP_SLOTS`): launcher + **at most 2** concurrent apps.  
2. **Full multi-app simultaneous** (launcher+SM+filemgr+calc+edit+IPC+input+net together): **NOT IMPLEMENTED** / not feasible under current caps. Integration process test states this explicitly.  
3. **External DNS** this run: **SKIPPED** (optional; env not set).  
4. **Interactive GUI** (click launch, drag/resize, physical keyboard/mouse end-to-end): **UNVERIFIED** in this acceptance pass — requires separate manual session.

---

## 3. Feature matrix (M20.0.1–M20.8)

Legend: **IMPLEMENTED** = code present; **PASS** = automated test evidence this run; **PARTIAL** = implemented with known gaps; **NOT IMPLEMENTED** = absent; **UNVERIFIED** = no adequate automated/manual proof this pass.

| Area | Implementation | Automated | Notes |
|---|---|---|---|
| M20.0.1 build / link integrity | IMPLEMENTED | PASS | `m20_build_integrity_tests` |
| M20.0.2 resource ownership | IMPLEMENTED | PASS | `m20_resource_integrity_tests` |
| M20.1 input evt queues / isolation | IMPLEMENTED | PASS | unit + process |
| M20.2 IPC send/recv/peek/available/overflow | IMPLEMENTED | PASS | unit + process |
| M20.3 WIN_DRAW_TEXT / bounds | IMPLEMENTED | PASS | unit + process |
| M20.4 window events / CLOSE_REQUEST / DESTROYED | IMPLEMENTED | PASS | unit + process (host/unit paths) |
| M20.5 launcher APP_INFO/RUN/CTRL | IMPLEMENTED | PASS | unit; process is smoke spawn |
| M20.5 interactive click-to-launch UX | PARTIAL / smoke only | — | `gui_launch.s` auto-spawns; not product UI |
| M20.6 service register/start/stop/restart | IMPLEMENTED | PASS | `m20_service_tests` |
| M20.7 FS CRUD seek/readdir/stat + filemgr process | IMPLEMENTED | PASS | unit + `m20_file_process_tests` |
| M20.8 integration + security host suite | IMPLEMENTED | PASS | `m20_integration_tests` + process |
| Concurrent full app set | NOT IMPLEMENTED | — | AS=3 |
| External live DNS | PARTIAL (optional) | SKIPPED | env gated |
| Physical viewer keyboard/mouse focus path | IMPLEMENTED (code) | UNVERIFIED | needs manual |
| `sys_wait` wake → zombie rescan | PARTIAL | — | code audit (`docs/m20-gui-runtime-investigation.md`); not a `make test` FAIL |

---

## 4. WAIT / lifecycle (audit only — no code change)

| Check | Status |
|---|---|
| EXIT path host reap (windows/ipc/evt/fd/sock/AS scrub) | IMPLEMENTED (mux) |
| WAIT finds existing ZOMBIE and frees PCB | IMPLEMENTED (`wt_scan` success path) |
| WAIT blocks then `wake_wait` → READY | IMPLEMENTED |
| After block-wake, **re-enter zombie scan before user resume** | **PARTIAL / defect** — `wt_block` advances EPC; wake does not re-run reap |
| Automated test dedicated to block-wake-reap | **UNVERIFIED** / none asserting this edge |
| Live dual occupancy of same AS slot | Not observed; spawn requires PCB FREE |

This WAIT gap is a **known issue for fix tracking**, not a failed binary in the acceptance `make test` log.

---

## 5. Defects & follow-ups

### Reproduced / code-confirmed (not make-test failures)

| Sev | Item | Action needed |
|---|---|---|
| MEDIUM | `sys_wait` after block may resume without reaping zombie (explains launcher `edit` on slot2) | Fix + add regression test (separate change request) |
| LOW | `gui_launch.s` is smoke, not interactive launcher | Product UX if required |
| — | External DNS | Run with `HAEUN_NET_EXTERNAL=1` when network policy allows |

### Manual verification still required

- App list readability in live viewer  
- Calculator via real user interaction (beyond auto-smoke spawn)  
- File Manager interactive CRUD  
- Focus, keyboard delivery to focused pid, mouse move/resize/close  
- Service control from a real UI (not only ECALL smoke)  
- Post-exit resource observation in viewer session  

---

## 6. Counts

| Category | Count |
|---|---|
| M20 automated targets executed (this run, listed §1.2) | **19** |
| M20 PASS | **19** |
| M20 FAIL | **0** |
| M20 SKIP | **0** |
| Optional non-M20 SKIP (external DNS case) | **1** |
| Full `make test` exit | **0** (PASS) |

---

## 7. Final statement

Automated regression for M20.0.1–M20.8, including M20 unit/process/integration suites under `make test`, **passed** on a clean rebuild (`make clean && make all && make test`, exit 0).

**Final status: AUTOMATED ACCEPTANCE PASSED**

Not claimed: **RELEASE ACCEPTED**. Manual GUI and physical input acceptance remain open; WAIT block-wake reclaim remains a tracked PARTIAL defect for a future approved fix.
