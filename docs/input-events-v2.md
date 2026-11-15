# Input Events v2 (M20.1)

## Model

- Shared KBD MMIO FIFO (`vm->kbd_rx`) is **unchanged** for M10 demos.
- M20 adds **per-pid event queues** (`haeun_evt_hub`) demuxed from focus.

## Event types

| Type | Value | Meaning |
|------|-------|---------|
| KEY_DOWN / KEY_UP | 1 / 2 | Scancode (break bit → UP) |
| MOUSE_MOVE / DOWN / UP | 3 / 4 / 5 | Window-local coords (`a`=lx, `b`=ly) |
| FOCUS / BLUR | 6 / 7 | Focus transitions |
| CLOSE | 8 | Chrome close before destroy |
| RESIZE | 9 | Resize-corner engagement |
| MOUSE_WHEEL | 10 | `a`=lx, `b`=signed delta |

Guest-visible record is fixed **12 bytes**: `type:u16`, `win:i16`, `a:u32`, `b:u32`.
FIFO order is the sequence guarantee; a separate sequence field is not part of the
register ABI. Modifier flags are reserved (not yet populated).

## Queue

- Capacity: **16** events per PID (`HAEUN_EVT_QUEUE`)
- Slots: **8** concurrent PID queues (`HAEUN_EVT_SLOTS`)
- Overflow: **drop oldest**, increment `drops`, log `[haeun-evt] queue overflow`
- Empty poll returns type `0` (not an error)

## Delivery

1. Viewer / input-host → `haeun_gfx_chrome_key` / `pointer` (or direct `haeun_evt_route_*`)
2. Hub looks up `wm.focus_id` → `owner_pid`
3. Event enqueued for that PID only
4. Guest `EVENT_POLL` (ECALL **27**) pops for **current pid only**

Host chrome drag/resize never injects client mouse events while a gesture is active.

## ECALL / libapp

| API | Notes |
|-----|-------|
| `app_event_poll` | ECALL 27; r1=type (0=empty), r2=win, r3=a, r4=b |
| `app_event_wait` | Cooperative: `YIELD` + poll loop (no new syscall) |
| `app_event_type` | Identity helper (r1 already holds type) |

No guest buffer pointer — register ABI only. `pid<=0` yields empty.
User MMIO keyboard mapping remains unavailable under process-v1 (no U-bit).

## Lifecycle

- `WIN_CREATE` (pid≠0): raise + focus + `FOCUS` event
- `WIN_DESTROY` / process reap: `haeun_evt_sync_focus`
- `EXIT` / M17 fault: `haeun_evt_reap(pid)` then sync focus
- PID reuse: slot cleared on reap so stale events cannot be polled

## Limits

- M20.2 IPC and M20.4 continuous window geometry streams are out of scope here.
- Physical keyboard/mouse require `--viewer` (manual); automated tests use scripts.
