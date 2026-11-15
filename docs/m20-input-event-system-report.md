# M20.1 Per-process Input Event System — Report

## Summary

HAEUN-OS routes host keyboard/mouse input to **per-PID bounded event queues**
using Window Manager focus. Shared KBD/MOUSE MMIO FIFOs remain for M10 demos;
process apps use `EVENT_POLL` (ECALL 27) only — no user MMIO.

## Event data structure

Fixed 12-byte little-endian record (`haeun_evt`):

| Field | Type | Meaning |
|-------|------|---------|
| `type` | u16 | See type table |
| `win` | i16 | Window id |
| `a` | u32 | Key scancode / mouse lx / wheel lx |
| `b` | u32 | Mouse ly / wheel signed delta |

Types: `KEY_DOWN=1`, `KEY_UP=2`, `MOUSE_MOVE=3`, `MOUSE_DOWN=4`, `MOUSE_UP=5`,
`FOCUS=6`, `BLUR=7`, `CLOSE=8`, `RESIZE=9`, `MOUSE_WHEEL=10`.

Register ABI (no guest pointer): `r1=type` (0=empty), `r2=win`, `r3=a`, `r4=b`.
FIFO order is the sequence guarantee. Modifier flags are reserved.

## Queue capacity and overflow

- Capacity: **16** events / PID (`HAEUN_EVT_QUEUE`)
- Concurrent PID slots: **8** (`HAEUN_EVT_SLOTS`)
- Overflow policy: **drop oldest**, increment `drops`, log
  `[haeun-evt] queue overflow (drop oldest)` — never reported as a successful
  delivery of the discarded event
- Empty: `haeun_evt_poll` / `EVENT_POLL` return 0 / type 0

## Routing structure

```
Host Input (viewer / input_host script)
  → haeun_gfx_chrome_key / pointer  (or haeun_evt_route_* when no chrome)
  → wm.focus_id → window.owner_pid
  → haeun_evt_push(pid, …)
  → guest app_event_poll (ECALL 27)
```

- Only the focused window’s owner receives keys/mouse/wheel.
- No focus → key/wheel logged and dropped.
- Chrome drag/resize gestures do not inject client app mouse events.
- `WIN_CREATE` (pid≠0): raise + focus + `FOCUS` event.
- Destroy / EXIT / fault: `haeun_evt_reap` + `haeun_evt_sync_focus`.

## ECALL and userland API

| Item | Value |
|------|-------|
| ECALL | `HAEUN_SYS_EVENT_POLL = 27` (unchanged) |
| Mux path | `haeun_m20_ecall_io` via `haeun_ecall_mux_io` |
| `app_event_poll` | Non-blocking; empty → r1=0 |
| `app_event_wait` | Cooperative `YIELD` + poll (no new syscall) |
| `app_event_type` | Identity helper |

Errors: `pid<=0` or missing hub → empty (not a crash). No user buffer to
validate for this ABI; IPC/draw paths retain VA checks separately.

## PID isolation

Verified in `tests/m20_unit_tests.c` and `tests/m20_input_tests.c`:

- Push/poll per PID; foreign PID cannot dequeue
- Focus change moves subsequent keys to the new owner
- BLUR delivered to the previous owner

## Process exit and PID reuse

- EXIT / M17 fault → `haeun_evt_reap(pid)` clears the slot
- Window destroy → `haeun_evt_sync_focus` so keys are not delivered to a dead focus
- After reap, a reused PID starts with an empty queue

## Test apps

| App | Source | Role |
|-----|--------|------|
| input_echo | `userland/app_input_echo.s` | KEY_DOWN → exit with scancode |
| mouse_tracker | `userland/app_mouse_trk.s` | MOUSE_DOWN → exit with lx |
| input_isolation | `userland/app_input_iso.s` | Empty queue / FOCUS-only at start |

Run via `process_sdk --app` (see `tests/m20_input_process_tests.py`).

## Automated test results

| Suite | Result |
|-------|--------|
| `m20_unit_tests` (isolation, overflow) | PASS |
| `m20_input_tests` (FIFO, focus, mouse/wheel, reap/reuse/destroy, EVENT_POLL) | PASS |
| `m20_input_process_tests` (3 apps + scripted `--input`) | PASS |
| M10 input | PASS |
| M11 viewer | PASS |
| M12 desktop | PASS |
| M17 fault | PASS |
| M19 SDK | PASS |
| M20.0.1 build integrity | PASS |
| M20.0.2 resource integrity | PASS |
| M20 process (text/ipc/files/calc) | PASS |

## Physical input (manual)

Not executed in this automated run. Manual check:

```
haeun-vm process_sdk.hxf --app build/app_input_echo.hxf --gui --desktop --viewer
```

Focus the guest window, press keys / move mouse / wheel; app should observe
events via `app_event_poll`. Document results separately from CI.

## Unimplemented / limits

- Modifier flags not populated
- Explicit sequence number not in register ABI (FIFO order only)
- `EVENT_WAIT` is cooperative, not a blocking kernel wait
- Continuous resize geometry stream deferred to M20.4
- IPC (M20.2) not implemented here
- Shared KBD MMIO path kept for M10; process apps must not rely on it

## Key files

- `include/haeun/evt.h`, `src/evt.c`
- `src/gfx_chrome.c`, `src/gfx_ecall.c`, `src/vm_input_host.c`, `src/vm_viewer_win32.c`
- `src/m20_ecall.c`, `src/ecall_mux.c`, `userland/libapp.s`
- `docs/input-events-v2.md`
