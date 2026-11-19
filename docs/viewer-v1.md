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

Three distinct mechanisms (see also [m22.4-manual-validation.md](m22.4-manual-validation.md)):

1. **Viewer live input** (`--viewer`): `haeun_vm_viewer_attach` enables `live_input`. Win32 `WM_KEY*` / mouse messages feed the guest (`src/vm_viewer_win32.c`).
2. **`--input`**: loads a host input script; without `--viewer`, runs `haeun_vm_input_host_run`.
3. **`--input` + `--viewer`**: the same script is attached as `viewer.script` and replayed in the viewer loop (still scripted, not physical HID).

Live window messages (IRQ left disabled for process-v1):

- `WM_KEYDOWN` / `WM_KEYUP` → `MapVirtualKey` scancode → keyboard feed + chrome/evt routing
- Mouse → client→FB map (`haeun_fb_view_map_mouse`) → mouse post / chrome pointer

## CLI

```text
haeun-vm build/process_desktop.hxf --apps --persist --viewer
haeun-vm build/gui_demo.hxf --gui --user --viewer
haeun-vm build/process_desktop.hxf --apps --persist --input --fb-dump out.ppm
```

Physical HID via the Win32 viewer path is **not** separately certified until a live manual checklist is filled. Scripted `--input` / `--input-script` runs are not physical HID PASS.
