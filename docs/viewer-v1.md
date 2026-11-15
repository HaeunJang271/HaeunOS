# Real-time GUI viewer v1 (M11)

Status: **Win32 GDI cooperative viewer is implemented** for `--viewer`. SDL2 is not required. Non-Windows builds link a stub that returns `HAEUN_VIEWER_UNSUPPORTED`.

## Rendering architecture

1. Guest writes RGBA8888 LE pixels into FB RAM (`BASE`, typically `0x10000` for `--gui`).
2. Host run loop (single thread):
   - apply optional M10 script events
   - step a slice of guest instructions (`VIEW_SLICE=256`)
   - `PeekMessage` / dispatch (live input)
   - `haeun_fb_view_snapshot_bgra` copies FB → BGRA staging buffer
   - `StretchDIBits` scales into the window
3. No VM/render concurrency → no FB data races; snapshot is a deliberate copy for DIB format conversion.

## Backend / dependencies

| Item | Detail |
|---|---|
| API | Win32 `user32` + `gdi32` |
| Link | `-luser32 -lgdi32` (Windows `haeun-vm` only) |
| Scale | default 8× (64×32 → 512×256 client content) |

## Input capture

Live window messages call existing M10 APIs with IRQ left disabled:

- `WM_KEYDOWN` / `WM_KEYUP` → `MapVirtualKey` scancode → `haeun_vm_keyboard_feed` (break = `code\|0x80`)
- Mouse move/buttons → client→FB map → `haeun_vm_mouse_post` (+ optional `haeun_gfx_wm_hit_focus`)

`--input --viewer` also replays the M10 demo script inside the viewer loop.

## CLI

```text
haeun-vm build/gui_demo.hxf --gui --user --viewer
haeun-vm build/input_demo.hxf --gui --user --input --viewer
haeun-vm build/gui_demo.hxf --gui --user --fb-dump out.ppm   # unchanged
```

## Not claimed

- SDL2 / cross-platform GUI toolkit
- Multi-threaded async rendering
- Enabling KBD/MOUSE IRQs
