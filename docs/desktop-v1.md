# Desktop & window manager v1 (M12)

## Phase 1 audit (pre-implementation)

| Feature | Status | Evidence |
|---|---|---|
| Window create/destroy/move/visible/raise | IMPLEMENTED | `gfx_wm.c` |
| Z-order compose | IMPLEMENTED | `gfx_comp.c` |
| focus_id / hit_focus | IMPLEMENTED | M10 `gfx_wm.h` |
| Graphics ECALL 18–23 | IMPLEMENTED | `gfx_ecall.c` |
| Desktop wallpaper + task strip | PARTIAL | `gfx_desk.c` (v4); no per-window chrome |
| Title bars / borders / buttons | NOT IMPLEMENTED | — |
| Drag / resize | NOT IMPLEMENTED | move API exists; no pointer gesture |
| Minimize / maximize / restore | NOT IMPLEMENTED | visible hide only |
| Close button | NOT IMPLEMENTED | destroy API exists |
| Keyboard → active window buffer | NOT IMPLEMENTED | focus_id only |
| Guest multi-window interactive demo | PARTIAL | `gui_demo` creates 3 windows, no chrome UX |

**WM model:** hybrid host-managed table; guest issues ECALLs; host composites and (M12) owns chrome/gestures.

## M12 architecture

- Guest: content backings + WIN_* / GFX_COMPOSE (ABI unchanged).
- Host `haeun_gfx_chrome`: decorations, drag/resize/min/max/close, key focus bookkeeping.
- Display size `w,h` may shrink within immutable backing `bw,bh` (no guest realloc).
- M11 viewer calls `haeun_gfx_chrome_pointer` instead of bare `hit_focus` when `--desktop` is set.
- KBD/MOUSE IRQ remain disabled.

## Deferred (M13/M14)

- App launcher / independent process-per-window
- Guest-driven decoration ECALLs
- Incremental dirty-region compositor
- Rich IME / text widgets
