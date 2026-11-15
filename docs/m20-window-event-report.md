# M20.4 Window Event API — Report

## Summary

Window lifecycle/focus events share the M20.1 per-PID queue and `EVENT_POLL`
(ECALL 27). New types 11–15 and 17 extend the model without changing the
12-byte register ABI. Soft CLOSE_REQUEST is separated from DESTROYED.
`WIN_QUERY` (ECALL 37) provides owner-only geometry/state.

## Event data structure

Unchanged guest layout (`haeun_evt`):

| Field | Size | Use |
|-------|------|-----|
| type | u16 | Input 1–5,10 vs window 6–9,11–15,17 |
| win | i16 | Window id |
| a, b | u32 | Key/mouse/geometry payload |

Sequence is FIFO order (no extra seq field in register ABI).

## Event kinds and meaning

See `docs/window-events-v1.md`. Aliases: FOCUS=FOCUSED, BLUR=UNFOCUSED,
CLOSE=CLOSE_REQUEST, RESIZE=RESIZED (on gesture complete).

## State-change emission conditions

| Change | When posted |
|--------|-------------|
| CREATED | After `WIN_CREATE` + owner |
| FOCUS/BLUR | `haeun_evt_on_focus` on real focus_id change |
| MOVED | Title-drag **mouseup** if x/y changed |
| RESIZED | Resize-corner **mouseup** if w/h changed |
| MINIMIZED/MAXIMIZED/RESTORED | Chrome caption / WM helpers |
| CLOSE | Close button (owned → no destroy) |
| DESTROYED | Before `WIN_DESTROY` clears slot |

No events for intermediate drag/resize samples.

## CLOSE_REQUEST vs DESTROYED

- **CLOSE (8):** request only for `owner_pid > 0`; window remains until
  `WIN_DESTROY` or process EXIT/reap.
- **DESTROYED (17):** window is going away; follows app destroy or host reclaim.
- Unowned chrome demos still destroy on close (M12 regression).

Unresponsive apps: window stays open; EXIT reaps windows and queues.

## Security

- Posts only to `owner_pid`; foreign PID cannot poll that queue.
- `WIN_QUERY` uses the same ownership check as other window ECALLs.
- Reap clears queues; PID reuse cannot see stale window events.

## Queue overflow

M20.1 policy: capacity 16, drop-oldest, `drops++`, stderr log.

## Userland API

| Symbol | Role |
|--------|------|
| `app_event_poll` | Shared poll (input + window) |
| `app_evt_iswin` | Window-type check |
| `app_win_query` | ECALL 37 geometry/flags |
| `app_event_type` | Identity helper |

## Test results

| Suite | Result |
|-------|--------|
| `m20_window_tests` (lifecycle, soft close, query, unowned close) | PASS |
| `m20_window_process_tests` (3 apps) | PASS |
| `gfx_chrome_tests` / M12 desktop | PASS |
| M20.1 input / M20.2 IPC / M20.3 text | PASS |

## Manual viewer

Not run in CI. Suggested:

```
haeun-vm process_sdk.hxf --app build/app_win_ev.hxf --gui --desktop --viewer
```

Drag title, resize corner, min/max/close; observe soft-close requires app destroy.

## Limits

- No separate window-only poll ECALL (shared queue by design).
- No blocking wait for window events.
- Sequence number not in register ABI.
- Soft-close has no host timeout force-destroy (EXIT/reap only).
- M20.5 launcher / M20.6 services not started.

## Key files

- `include/haeun/evt.h`, `src/evt.c`, `src/gfx_chrome.c`, `src/gfx_ecall.c`
- `src/m20_ecall.c` (`WIN_QUERY`), `userland/libapp.s`, `userland/app_win_*.s`
- `docs/window-events-v1.md`
