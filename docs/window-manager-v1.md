# HAEUN-OS Window Manager v1

Date: 2026-11-16  
Status: Stabilized (M23.1–M23.4)  
ISA/ABI: HAEUN-32 unchanged; guest window ECALLs 18–23, 27–28, 37, 51.

## Architecture

```
Guest apps / desktop shell
        │ ECALL (WIN_*, EVENT_POLL, GFX_COMPOSE, …)
        ▼
┌───────────────────┐     ┌─────────────┐
│ gfx_ecall / m20   │────▶│ evt hub     │
└─────────┬─────────┘     └──────▲──────┘
          ▼                      │
┌───────────────────┐     ┌──────┴──────┐
│ haeun_gfx_wm      │◀────│ gfx_chrome  │◀── input-host / viewer
│ table, z, focus   │     │ gestures    │
└─────────┬─────────┘     └─────────────┘
          ▼
┌───────────────────┐
│ haeun_gfx_compose │──▶ primary FB (desktop: 320×180)
└───────────────────┘
```

Core types live in `include/haeun/gfx_wm.h`. Chrome is host-only UI chrome (title, min/max/close, drag/resize). Events are demuxed per `owner_pid` in `haeun_evt_hub`.

## Window lifecycle

| Step | Host API | Guest | Notes |
|------|----------|-------|-------|
| Create | `haeun_gfx_wm_create` | `WIN_CREATE` (18) | Sets `used`, `visible`, `z=next_z++`; guest path sets owner, raise, focus, `EVT_CREATED` |
| Show | `haeun_gfx_wm_set_visible(...,1)` | — | Clears `minimized` |
| Hide | `haeun_gfx_wm_set_visible(...,0)` | — | Focus recover |
| Minimize | `haeun_gfx_wm_minimize` | chrome only | `minimized=1`, `visible=0`, focus recover |
| Restore | `haeun_gfx_wm_restore` | chrome max toggle / host | Un-minimizes or un-maximizes |
| Raise | `haeun_gfx_wm_raise` | `WIN_RAISE` (22) | `z=next_z++`; guest raise also claims focus |
| Focus | `haeun_gfx_wm_set_focus` | via raise / chrome | Rejects min/hidden |
| Destroy | `haeun_gfx_wm_destroy` | `WIN_DESTROY` (19) | Clears slot; focus recover; `EVT_DESTROYED` before destroy |
| Owner exit | `haeun_app_destroy_windows` | `EXIT` → mux reap | All owned slots destroyed; evt/ipc/svc reaped |

## Focus eligibility

A window may hold `focus_id` only when:

`used && visible && !minimized`

- `haeun_gfx_wm_focus_recover` / `top_eligible`: retarget to top eligible by **z**, preferring **`owner_pid > 0`** over unowned shell surfaces; else `-1` (documented no-focus).
- `haeun_evt_route_key` / mouse / wheel: drop if focus ineligible.
- `haeun_gfx_chrome_key`: recover before routing when focus is stale.

## Z-order behavior

- Each create/raise assigns `z = next_z++` (monotonic).
- Compose blits ascending `z` (highest last = on top).
- Minimized windows are **not** composed (even if `visible` were inconsistent).
- Hit-test / chrome `top_zone`: among windows containing the point, prefer **non-shell** (or owned) over fullscreen unowned root, then highest `z`.

## Input routing

- Keyboard: `haeun_evt_route_key` → focused window’s `owner_pid` queue (`KEY_DOWN`/`KEY_UP`).
- Mouse/wheel: focused eligible window’s client area only.
- Unowned / no-focus: keys dropped (logged).
- Isolation: other PIDs do not receive the event.

## Chrome event handling

| Zone | Owned (`owner_pid > 0`) | Unowned |
|------|-------------------------|---------|
| Close | Soft `EVT_CLOSE` only (slot kept) | Destroy immediately |
| Min | Minimize + `EVT_MINIMIZED` + focus sync | Same |
| Max | Maximize / restore geometry events | Same |
| Title / client | Focus + raise; drag/resize for non-shell | Same |

Desktop shell surfaces (`w/h` covering FB) skip caption chrome painting and drag.

## Guest APIs (stable ABI)

| ECALL | Name | Role |
|-------|------|------|
| 18 | `WIN_CREATE` | Create in user pool; owned |
| 19 | `WIN_DESTROY` | Destroy own window |
| 20 | `WIN_FILL` | Solid fill |
| 21 | `WIN_PIXEL` | Put pixel |
| 22 | `WIN_RAISE` | Raise + focus |
| 23 | `GFX_COMPOSE` | Guest compose (viewer may also `chrome_present`) |
| 27 | `EVENT_POLL` | Non-blocking event pop |
| 28 | `WIN_DRAW_TEXT` | Glyph text |
| 37 | `WIN_QUERY` | Geometry + flags (focus/min/max/visible) |
| 51 | `WIN_FILL_RECT` | Rect fill |

No guest move / minimize / maximize ECALLs in v1 (chrome gestures only).

## Window capacity

- `HAEUN_GFX_WM_MAX_WINDOWS = 8`.
- Desktop typically consumes **2** slots (root + strip).
- Create failure when table full (`HAEUN_GFX_BAD_ARGUMENT` / guest `-1`).
- Destroy clears `used` and `owner_pid` (slot reusable).

## Known limitations

- Physical HID not validated by automation (`--input-script` / host unit only).
- Dual compose: guest `GFX_COMPOSE` and viewer `chrome_present` may both run; viewer path is authoritative under `--viewer`.
- Task-strip click **raises** a client but does **not** un-minimize; restore of minimized is `haeun_gfx_wm_restore` / chrome max path for maximized.
- Soft CLOSE has no host timeout-destroy.
- No dirty-rect invalidation; full-frame compose at 320×180.
- Drag/resize not clamped to FB (UX polish deferred).

## Untested / out of scope for v1 automation

- Physical keyboard/mouse on real hardware.
- Viewer-only Win32 pump latency / vsync.
- Simultaneous >2 interactive apps beyond desktop slot policy.
- Network/filesystem interaction with WM (orthogonal).
