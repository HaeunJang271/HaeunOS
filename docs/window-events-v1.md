# Window Events v1 (M20.4)

Delivered through the **same per-pid queue** as M20.1 input (`EVENT_POLL` / ECALL 27).
Distinguish by `type`; layout remains `type:u16, win:i16, a:u32, b:u32`.

## Types

| Type | Id | Meaning | a / b |
|------|----|---------|-------|
| FOCUS | 6 | WINDOW_FOCUSED | — |
| BLUR | 7 | WINDOW_UNFOCUSED | — |
| CLOSE | 8 | WINDOW_CLOSE_REQUEST | — |
| RESIZE | 9 | WINDOW_RESIZED (gesture **complete**) | w / h |
| CREATED | 11 | WINDOW_CREATED | x / y |
| MOVED | 12 | WINDOW_MOVED (drag **complete**) | x / y |
| MINIMIZED | 13 | | — |
| MAXIMIZED | 14 | | w / h |
| RESTORED | 15 | | w / h |
| DESTROYED | 17 | after WIN_DESTROY | — |

Input types 1–5 and 10 (`MOUSE_WHEEL`) are never window events.
`haeun_evt_is_window()` / `app_evt_iswin` classify types.

## Emission rules

- Emit only when state **actually changes** (no per-pixel drag spam).
- MOVED / RESIZED fire on **mouse release** after title-drag / resize-corner.
- CREATED then FOCUS on process `WIN_CREATE`.
- DESTROYED posted **before** slot clear on `WIN_DESTROY`.

## CLOSE_REQUEST vs DESTROYED

| | CLOSE (8) | DESTROYED (17) |
|--|-----------|----------------|
| Source | Chrome close button | `WIN_DESTROY` / reap path |
| Window | Still exists (owned apps) | Slot freed |
| Policy | Soft-close if `owner_pid > 0` | App or EXIT must destroy |

Unowned windows (host demos): close still destroys immediately (M12).
If an app ignores CLOSE_REQUEST, the window stays until `WIN_DESTROY` or process EXIT
(reap destroys windows).

## WIN_QUERY (ECALL 37)

Owner-only: `r2=id` → `r1=0`, `r2=x`, `r3=y`, `r4=(w|(h<<16))`, `r5=flags`
(bit0 focus, bit1 min, bit2 max, bit3 visible).

## libapp

- `app_event_poll` — shared input + window queue
- `app_evt_iswin` — window-type predicate
- `app_win_query` — ECALL 37

## Queue policy

Same as M20.1: depth 16, drop-oldest + `drops` on overflow.
