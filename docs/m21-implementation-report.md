# M21 Implementation Report — Persistent Desktop

Date: 2026-11-15  
Status: AUTOMATED PASS; live Win32 viewer path exercised (see manual doc).

## Summary

HAEUN-OS guest PCB0 now runs `userland/desktop.s` (`process_desktop.hxf`) as a persistent shell. Application EXIT no longer ends the OS; only desktop Esc → `SYS_EXIT(0)` (+ empty READY) halts the guest. M20 `gui_launch` / `process_launch.hxf` retained for regression.

## Architecture choices (from Phase 1)

| Topic | Choice |
|---|---|
| Persistent loop owner | Guest `desktop.s` (not viewer) |
| Idle | `EVENT_POLL` + `YIELD` + periodic `GFX_COMPOSE` (not SLEEP-alone) |
| App return | `APP_SPAWN` → `WAIT` → reclaim focus → loop |
| Shutdown | Esc scancode `0x01` → `EXIT(0)` |
| Launcher | Keys `1`/`2`/`3`/`4` (calc/edit/**files**/NOSPC) |
| ABI | Unchanged ECALL 0–50 |

## Artifacts

| Path | Role |
|---|---|
| `userland/desktop.s` | Persistent desktop guest |
| `tools/build_process_desktop.py` | Links `process_desktop.hxf` |
| `src/vm_input_host.c` | `haeun_vm_input_host_load_m21_script` |
| `src/main.c` | `--persist` selects M21 script |
| `src/gfx_ecall.c` | `WIN_RAISE` also claims focus |
| `src/gfx_chrome.c` | Key routing adopts topmost window if `focus_id < 0` |
| `tests/m21_persistent_desktop_tests.py` | Automated acceptance |
| `docs/m21-architecture.md` | Phase 1 design |
| `docs/m21-manual-validation.md` | Live viewer checklist |

## Behavior notes

- **Idle:** Cooperative YIELD when the event queue is empty. Not an unbounded tight spin without yield; not timer-SLEEP (sole-process SLEEP gap remains PARTIAL).
- **Slots:** Still max 2 concurrent apps (`HAEUN_APP_SLOTS=3` with desktop on slot 0). Key `4` exercises NOSPC.
- **Focus:** After app `WIN_DESTROY`, focus may clear; desktop `WIN_RAISE` + chrome fallback restore KEY delivery without fabricating guest queue events.
- **Compatibility:** M20 launcher path unchanged; M21 test re-runs `process_launch.hxf --apps`.

## Automated results

`tests/m21_persistent_desktop_tests.py`:

1. Desktop starts (clean run / compose / PPM).
2. Remains alive until Esc (≥700000 steps before halt).
3–5. calc / edit / files spawn markers present.
6. Repeated calc spawn (≥2).
7. NOSPC log present.
8. WAIT/reap implied by multi-launch without OS halt.
9. `[haeun-evt] key … sc=0x0[1-5] down` observed.
10. Mouse posts / focus path exercised in input-host summary.
11. Guest not halted before Esc timeline (viewer-alive proxy headless).
12. Explicit shutdown: `guest halted` + `r1=0x00000000`.
13. M20 `process_launch` still clean-halts with spawn markers.

## PARTIAL / UNVERIFIED

| Item | Status |
|---|---|
| Timer-backed SLEEP idle with sole READY process | PARTIAL — not used; would need scheduler assist |
| Physical keyboard/mouse via live Win32 viewer | See manual checklist (not proven by automated script alone) |
| More than 2 simultaneous apps | Unsupported (AS capacity) |

## How to run

```text
python tools/build_process_desktop.py build/process_desktop.hxf
# build builtin apps into build/ as for M20
haeun-vm build/process_desktop.hxf --apps --persist --input
haeun-vm build/process_desktop.hxf --apps --persist --viewer
python tests/m21_persistent_desktop_tests.py build/haeun-vm.exe
```
