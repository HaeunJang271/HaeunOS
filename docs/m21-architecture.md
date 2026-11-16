# M21 Architecture — Persistent Desktop

Date: 2026-11-15  
Status: design for implementation (inspect-confirmed).

## 1. Ownership of the persistent loop

| Candidate | Role | Decision |
|---|---|---|
| `firmware/boot.s` | Firmware only | Not used for desktop |
| `proc_boot.s` / scheduler | Kernel PCB0 enters user `user_init` | Keeps providing process substrate |
| **Guest `desktop.s` (PCB0)** | Userland desktop shell | **Owns** event loop, spawn/wait, shutdown |
| Host viewer | Cooperative step + Win32 pump + FB blit | Stays open while guest ≠ halted |
| `gui_launch.s` | M20 smoke (EXIT+halt) | **Retained** for M20 tests; not the M21 desktop |

**Choice:** persistent loop lives in guest **`userland/desktop.s`**, linked as `process_desktop.hxf`. No ABI change.

## 2. Scheduler support for idle desktop

- PCB0 = desktop AS slot 0; apps use slots 1–2 (`HAEUN_APP_SLOTS=3`).
- `YIELD`: sole READY process re-picks itself — safe idle building block.
- `SLEEP`: when no other READY PCB, `pick_next` returns none; `sys_sleep` still `restore_ret` — **unsafe as sole-process idle** without kernel change.
- `WAIT`: blocks desktop until child zombie (M20.9 re-scan fix) — used **after spawn** so app EXIT returns to desktop without OS halt.

**Idle policy (MVP):** `EVENT_POLL` → if empty, `YIELD` (and periodic `GFX_COMPOSE`). Not a tight spin without yield; not SLEEP-alone. Documented PARTIAL vs timer-backed idle.

## 3. Viewer input while guest runs

`haeun_vm_viewer_run` (`vm_viewer_win32.c`): step `VIEW_SLICE` → `pump()` → `refresh_fb()`.  
`--apps --desktop` attaches chrome → keys/mouse → `haeun_evt_route_*` to focused window owner.  
Desktop window owns focus when no app; after app `WIN_CREATE`, app takes focus; after app EXIT/reap, desktop can reclaim focus on next create/raise.

## 4. App → desktop return

1. Desktop `APP_SPAWN(index)`.
2. Desktop `WAIT` (reaps zombie; host mux already scrubbed on EXIT).
3. Loop resumes: paint/compose, poll input, accept new launches.

No guest `halt` on app EXIT (`sys_exit` finds desktop READY).

## 5. Explicit OS shutdown

- Desktop-only convention: **Esc** scancode `0x01` → `SYS_EXIT(0)` (`HAEUN_SYS_EXIT=0`) then `halt`.
- Not an ABI extension; apps may also EXIT but only empty READY set + halt ends OS.
- Host `--persist` loads an input script that ends with Esc for automated shutdown.

## 5b. Focus reclaim (host behavior note)

- `WIN_RAISE` (ECALL 22) now also `set_focus` + `evt_on_focus` for the raised window.
- Register ABI unchanged; needed so desktop regains KEY routing after app `WIN_DESTROY` clears `focus_id`.

## 6. Efficient idle / WAIT / events

| Mechanism | Use in M21 |
|---|---|
| `EVENT_POLL` | Primary desktop input |
| `YIELD` | Idle when queue empty |
| `WAIT` | After each app spawn |
| `SLEEP` | Deferred (sole-process gap) |
| Host script | Deterministic E2E without fake evt injection into guest queues beyond existing input-host |

## 7. Launch surface

Keys (US set-1 scancodes):

| Key | Scancode | Action |
|---|---|---|
| `1` | `0x02` | Spawn calc (index 0) |
| `2` | `0x03` | Spawn edit (index 1) |
| `3` | `0x04` | Spawn **files** (index **2**; not `filemgr`) |
| `4` | `0x05` | NOSPC probe (then CLOSE fillers) |
| Esc | `0x01` | Shutdown |

NOSPC: dual-fill attempt logged via existing `app-env: spawn NOSPC`.

## 8. Compatibility

- M20 `process_launch.hxf` / `gui_launch.s` unchanged in role.
- ECALL 0–50 unchanged.
- New artifacts: `desktop.s`, `build_process_desktop.py`, `--persist`, M21 tests/docs.
