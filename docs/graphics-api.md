# Graphics API (current runtime)

Synced with verified host/guest behavior as of M22.2.  
See also [m22-desktop-specification-audit.md](m22-desktop-specification-audit.md), [m22.1.3-target-desktop.md](m22.1.3-target-desktop.md).

## Host geometry

Default `--gui` / `--apps` / desktop framebuffer (**current**):

| Property | Value |
|----------|--------|
| Size | **320×180** |
| PA | **`0x00200000`** |
| Constants | `HAEUN_GUI_FB_*` in `include/haeun/vm_gui_host.h` |

Window backings for process apps use the pool `[0x18000, 0x100000)` (`HAEUN_WIN_POOL_*`). The primary FB sits above that pool.

Viewer (`--viewer`): Win32 GDI; default scale **2** (~640×360 client). Mouse client coordinates map to FB pixels via `haeun_fb_view_map_mouse`.

## Guest ECALL (graphics / input surface)

| # | Call | Notes |
|---:|---|---|
| 18 | WIN_CREATE | sets `owner_pid` for process apps |
| 19 | WIN_DESTROY | owner only |
| 20 | WIN_FILL | owner only; full backing |
| 21 | WIN_PIXEL | owner only |
| 22 | WIN_RAISE | owner only; also claims focus |
| 23 | GFX_COMPOSE | blit WM → primary FB (no host chrome) |
| 27 | EVENT_POLL | per-pid event queue (keys/mouse/window) |
| 28 | WIN_DRAW_TEXT | ASCII 8×8 glyphs; wrap/clip in window |
| 37 | WIN_QUERY | geometry / flags |
| 51 | WIN_FILL_RECT | owner only; clipped rect fill |

Under `--viewer`, each refresh also runs `haeun_gfx_chrome_present` (compose + host title chrome on **non-shell** windows). A fullscreen shell (`w≥FB_w` and `h≥FB_h`) skips host chrome and is pinned at `(0,0)`.

## Shell rendering ownership

Desktop header, welcome text, **three app tiles**, and bottom bar are **pixels inside one guest shell window** (backing e.g. `0x20000`), not separate WM windows. App windows (calc/edit/files) are additional WM surfaces; host chrome may decorate those.

## Intentional limits

- Glyphs: 8×8 ASCII only (Hangul unsupported).
- Shared KBD FIFO still exists for legacy demos; process desktop input uses **EVENT_POLL** queues.
- AS ≤ 3, app image ≤ 4KiB @ `0x00108000`, max 8 windows.

Historical 64×32 notes remain in older milestone reports only.
