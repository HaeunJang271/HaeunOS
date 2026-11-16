# Real-time GUI viewer (Win32)

Status: **Win32 GDI cooperative viewer** for `--viewer`. Non-Windows builds link a stub (`HAEUN_VIEWER_UNSUPPORTED`).

Synced M22.2 with current FB geometry.

## Rendering architecture

1. Guest draws into **window backings**; compose writes RGBA8888 LE into primary FB RAM (`BASE` = **`0x00200000`**, **320×180** for default `--gui` / desktop).
2. Host run loop (single thread):
   - optional scripted input events
   - step a slice of guest instructions (`VIEW_SLICE`)
   - `PeekMessage` / dispatch (live input)
   - `haeun_gfx_chrome_present` when chrome is attached (compose + host decorations)
   - `haeun_fb_view_snapshot_bgra` copies FB → BGRA staging
   - `StretchDIBits` scales into the window
3. No VM/render concurrency → no FB data races.

## Backend / dependencies

| Item | Detail |
|---|---|
| API | Win32 `user32` + `gdi32` |
| Link | `-luser32 -lgdi32` (Windows `haeun-vm` only) |
| Scale | default **2×** (320×180 → ~640×360 client) |

## Input capture

Live window messages (IRQ left disabled for process-v1):

- `WM_KEYDOWN` / `WM_KEYUP` → `MapVirtualKey` scancode → keyboard feed + chrome/evt routing
- Mouse → client→FB map (`haeun_fb_view_map_mouse`) → mouse post / chrome pointer

`--input --viewer` can also replay host input scripts inside the viewer loop.

## CLI

```text
haeun-vm build/process_desktop.hxf --apps --persist --viewer
haeun-vm build/gui_demo.hxf --gui --user --viewer
haeun-vm build/process_desktop.hxf --apps --persist --input --fb-dump out.ppm
```

Physical HID beyond the Win32 viewer path is **not** separately certified.
