# HAEUN-OS M22: Desktop Specification Audit

Date: 2026-11-16  
Mode: **READ-ONLY** (no source changes in this pass)  
Scope: README 1.0 claims + graphics architecture vs live desktop guest/host path.

**Follow-up:** Documentation sync for D1–D3 (and related stale maps) is recorded in [m22.2-documentation-sync-report.md](m22.2-documentation-sync-report.md).

## 1. Documented behavior

| Source | Claim |
|--------|--------|
| `README.md` | FB **320×180**; `process_desktop.hxf --apps --persist --viewer`; tiles Calculator / Editor / Files; keys **1–3**; Esc quits shell; apps persistent until Esc/CLOSE; interactive calc/edit |
| `docs/m22.1.3-target-desktop.md` | FB 320×180 @ `0x00200000`; shell backing `0x20000`; header / welcome / **three tiles** / guest taskbar; shell skips host chrome; pinned (0,0) |
| `docs/m22.1-ui-architecture.md` | (**synced M22.2**) Current = 320×180 / keys 1–3 → calc/edit/files; HISTORICAL 64×32 sketch isolated |
| `docs/graphics-api.md` | (**synced M22.2**) Default FB **320×180** @ `0x00200000`; ECALL 27/28/51 documented |
| `docs/architecture.md` / `docs/limitations.md` | (**synced M22.2**) FB 320×180; EVENT_POLL / WIN_DRAW_TEXT available; supported vs partially tested |

## 2. Actual implemented behavior

### 2.1 Geometry and host attach

| Item | Value | Evidence |
|------|--------|----------|
| FB W×H | **320×180** | `HAEUN_GUI_FB_*` in `include/haeun/vm_gui_host.h` |
| FB PA | **`0x00200000`** | same; `haeun_vm_gui_host_attach_ex` → `haeun_gfx_fb_enable` |
| Window pool | `[0x18000, 0x100000)` | `HAEUN_WIN_POOL_*` in `include/haeun/gfx_wm.h` |
| Shell window | `(0,0)` **320×180**, backing **`0x20000`** | `userland/desktop.s` `user_init` |
| Viewer scale | 2 (~640×360 client) | `haeun_vm_viewer_attach` in `src/vm_viewer_win32.c` |
| Mouse map | client → FB via integer scale | `haeun_fb_view_map_mouse` in `src/fb_view.c` |

### 2.2 Guest shell (PCB0)

| Behavior | Implementation |
|----------|----------------|
| Paint | `shell_paint` / `shell_tile` / `shell_fill_rect` / `shell_text` in `userland/desktop_draw.s` |
| Compose (guest) | `shell_compose` → ECALL **23** `GFX_COMPOSE` |
| Tiles (once per paint) | Three `shell_tile` calls: Calc `(16,88)`, Edit `(112,88)`, Files `(208,88)`, size **96×56** |
| Hit tests | `on_mouse` in `desktop.s`: Calc `[16,112)×[88,148)`, Edit `[112,208)×…`, Files `[208,304)×…` |
| Keys | `1`→calc(0), `2`→edit(1), `3`→files(**2**), `4`→NOSPC probe, Space→menu flag, **Esc→shutdown** |
| Spawn policy | `APP_SPAWN` then **no WAIT**; busy debounce; apps persist |
| Shutdown | `close_all` (`APP_RUN`+`APP_CTRL CLOSE`+`WAIT` per child) then destroy shell + `EXIT(0)` |

### 2.3 Builtin apps (`--apps`)

| Index | Name | Guest | Notes |
|------:|------|-------|--------|
| 0 | calc | `userland/app_calc.s` | Interactive digits/ops; Esc/CLOSE |
| 1 | edit | `userland/app_edit.s` | Interactive type/backspace; Esc/CLOSE |
| 2 | files | `userland/app_files.s` | Registry list smoke; Esc/CLOSE |
| 3 | svcidle | service path | Not a desktop tile |
| 4 | filemgr | `app_filemgr.s` | Launcher/HFS tests; **not** Files tile |

### 2.4 Events / text / chrome

| API | Status in code |
|-----|----------------|
| `EVENT_POLL` (27) | Implemented (`src/m20_ecall.c`, `src/evt.c`) |
| `WIN_DRAW_TEXT` (28) | Implemented; 8×8 font; wrap/clip in `src/gfx_ui.c` |
| `WIN_FILL_RECT` (51) | Implemented (`src/gfx_ecall.c`, `src/gfx_wm_fill.c`) |
| Host title chrome | Drawn for **non-shell** windows in `paint_one` (`src/gfx_chrome.c`) |
| Shell chrome skip | `is_shell`: `w≥FB_w && h≥FB_h` → no title bar; pin `(0,0)` in `haeun_gfx_chrome_present` |

## 3. Rendering ownership

```text
                    ┌─────────────────────────┐
                    │  Guest PCB0 desktop.s   │
                    │  paints shell backing   │
                    │  @ PA 0x20000 (320×180) │
                    └───────────┬─────────────┘
                                │ WIN_FILL / FILL_RECT / DRAW_TEXT
                                │ GFX_COMPOSE (ECALL 23)
                                ▼
┌──────────────┐    ┌─────────────────────────┐
│ App PCBs     │───▶│  WM window backings     │
│ calc/edit/…  │    │  (pool, e.g. 0x60000+)  │
└──────────────┘    └───────────┬─────────────┘
                                │
         viewer slice refresh   │  guest compose (no decorations)
                │               ▼
                │     haeun_gfx_compose → primary FB @ 0x00200000
                ▼
    haeun_gfx_chrome_present (`src/gfx_chrome.c`)
         1) pin shell to (0,0)
         2) haeun_gfx_compose (clear + blit windows)
         3) paint_one host chrome on non-shell windows
                │
                ▼
    haeun_fb_view_snapshot_bgra → StretchDIBits (`vm_viewer_blit.c`)
```

**Idle screenshot layers (one WM window only):**

| Visible UI | Owner | WM window? |
|------------|-------|------------|
| Header / welcome / 3 tiles / bottom bar | Guest `shell_paint` pixels | **No** — all inside shell surface |
| Host title/min/max/close strip | N/A on idle | Shell skipped by `is_shell` |

**With app open:** shell surface (z lower) + app surface + **host** chrome around the app.

## 4. Coordinate / tile / text checks

| Check | Result |
|-------|--------|
| 320×180 FB vs guest create | Match (`desktop.s` + `vm_gui_host.h`) |
| Three tiles drawn once per `shell_paint` | Confirmed — three sequential `shell_tile` sites |
| Tile draw vs hit boxes | Aligned to 96×56 origins 16/112/208 @ y=88 |
| Glyph size | 8×8 (`HAEUN_GFX_UI_GLYPH`); OOB pixels skipped |
| Text clip | Wrap on width; stop when `cy ≥ win->h` (`draw_text_win`) |
| Mouse FB coords | Viewer maps client→FB; shell at (0,0) ⇒ local≈screen |

## 5. Keyboard / mouse interaction (verified by code + tests)

| Path | Works? | Notes |
|------|--------|--------|
| Keys 1–3 spawn | Yes | `m21` / `m22_1` scripts + asserts |
| Esc shell quit | Yes | Requires shell focus (script raises header first) |
| Tile click spawn | Yes | `on_mouse` → `sp_*` |
| App Esc close | Yes | Apps handle KEY_DOWN sc=`0x01` / CLOSE |
| Interactive calc/edit | Code yes | **M22.3** `m22_3_interactive_app_tests.py` (scripted keys + PPM OCR) |
| Focus | Host chrome/evt | App `WIN_CREATE` raises+focuses; Esc to shell needs raise/click |

## 6. Confirmed discrepancies

| ID | Severity | Documented | Actual |
|----|----------|------------|--------|
| D1 | **Resolved (M22.2)** | Was: FB 64×32; no DRAW_TEXT/POLL | Canonical docs now match **320×180** + ECALL 27/28/51 |
| D2 | **Resolved (M22.2)** | Was: WAIT after spawn; Files=`count-1`→filemgr | Docs: no WAIT; Files tile → **index 2 `files`** |
| D3 | **Resolved (M22.2)** | Was: older 64×32 sketch as current | Current section = M22.1.3+; 64×32 under HISTORICAL |
| D4 | **Resolved (M22.3)** | Was: interactive calc/edit untested | Guest scripted tests + PPM OCR — [m22.3-interactive-app-tests.md](m22.3-interactive-app-tests.md) |
| D5 | **Low (docs)** | README: keys 1–3 only | Also key **4** NOSPC, **Space** menu flag (`desktop.s`) |
| D6 | **Resolved (M22.4)** | Was: docstring “64-wide” | Comment updated to 320×180 |
| D7 | **Info (architecture)** | Single compose owner | Dual: guest ECALL 23 **and** viewer `chrome_present` each slice |

Not discrepancies (justified by existing design): guest-drawn taskbar strip; host chrome on floating apps; AS≤3 / app≤4KiB caps.

## 7. Release-contract vs tests

| Claim | Runtime evidence | Automated coverage |
|-------|------------------|--------------------|
| 320×180 desktop | Host enum + guest create + PPM samples | `tests/m22_1_desktop_shell_tests.py`, `m22_1_1_*`, `m22_1_2_*` |
| Persist | `--persist` unbounded budget | `m21_*`, `m22_1` STEP_LIMIT@11M |
| Launch calc/edit/files | spawn logs | `m21` asserts `spawn files` |
| Interactive calc/edit | Manual / viewer | **Missing** |
| Canonical docs vs 320×180 | Synced (M22.2) | Docs not gated by tests; historical milestone bodies retained |

## 8. Minimal correction sequence

Ordered for doc/test honesty first; no new WM/UI features.

1. ~~**Refresh Release 1.0 graphics docs**~~ — **Done in M22.2** ([m22.2-documentation-sync-report.md](m22.2-documentation-sync-report.md)).

2. ~~**Mark or rewrite `docs/m22.1-ui-architecture.md`**~~ — **Done in M22.2**.

3. **Align README extras** — optional one-liner for key `4` NOSPC / Space menu, or leave as power-user.  
   **Tests:** none.

4. **Fix stale test comment** — `read_ppm_rgb` docstring in `m22_1_desktop_shell_tests.py` (source; out of M22.2 doc-only scope).  
   **Tests:** same file still PASS.

5. ~~**Add minimal interactive app checks** (D4)~~ — **Done in M22.3** (`tests/m22_3_interactive_app_tests.py`).

6. ~~**Compose ownership note**~~ — Documented in M22.2 `graphics-api.md` / `viewer-v1.md` (viewer `chrome_present` vs guest `GFX_COMPOSE`).

## 9. Exact source index

| Role | Path |
|------|------|
| FB constants | `include/haeun/vm_gui_host.h` |
| GUI attach | `src/vm_gui_host.c` |
| Compose | `src/gfx_comp.c` (`haeun_gfx_compose`) |
| Chrome present / shell detect | `src/gfx_chrome.c` (`is_shell`, `paint_one`, `haeun_gfx_chrome_present`) |
| Pointer / shell skip | `src/gfx_chrome_ptr.c` |
| Text draw/clip | `src/gfx_ui.c`, `src/gfx_font.c`, `src/m20_ecall.c` |
| FILL_RECT | `src/gfx_ecall.c`, `src/gfx_wm_fill.c` |
| Viewer refresh | `src/vm_viewer_blit.c` (`haeun_viewer_refresh_fb`) |
| Mouse map | `src/fb_view.c` |
| Shell guest | `userland/desktop.s`, `userland/desktop_draw.s` |
| Apps | `userland/app_calc.s`, `app_edit.s`, `app_files.s` |
| Image link | `tools/build_process_desktop.py` |
| Acceptance | `tests/m21_persistent_desktop_tests.py`, `tests/m22_1_*.py` |

## 10. Verdict

The **running** M22 desktop matches the README / M22.1.3+ target (320×180 tile shell, persistent apps, Esc, viewer). **D1–D3** closed in M22.2; **D4** closed in M22.3 (scripted guest input + PPM OCR). No evidence that a new taskbar/WM/wallpaper subsystem is required; the bottom bar is guest framebuffer content inside the single shell window.
