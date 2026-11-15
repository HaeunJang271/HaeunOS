# M20.3 GUI Text Rendering — Report

## Summary

Userland apps draw ASCII into **owned window backings** via existing
`WIN_DRAW_TEXT` (ECALL 28). The 8×8 bitmap font, newline/auto-wrap layout,
transparent background, ownership checks, and pixel-level tests are in place.
UART console (`app_putc`) remains separate.

## Font format

| Item | Value |
|------|-------|
| Glyph size | **8×8** fixed |
| Storage | `static const uint8_t FONT8[95][8]` in `gfx_font.c` |
| Index | ASCII 32–126 → `FONT8[ch-32]` |
| Bit order | Bit7 = leftmost pixel |
| Replacement | Non-printable → `'?'` glyph (draw_char / draw_text) |
| Transparent bg | `bg == 0` skips background pixels |

## Text rendering architecture

```
app_draw_text / app_draw_char
  → ECALL 28 WIN_DRAW_TEXT
  → VA check + length cap + ownership
  → haeun_gfx_ui_draw_text / draw_char
  → haeun_gfx_wm_put_pixel (window-local; OOB clipped)
  → compose → primary FB (viewer)
```

Window-local coordinates; chrome decorations unchanged.

## ECALL number and ABI

| Call | # | Notes |
|------|---|-------|
| WIN_DRAW_TEXT | **28** | Unchanged ABI: id,x,y,fg,bg,strVA |
| (no new numbers) | | SYS_MAX remains 36 from M20.2 |

Errors: `ffffffff` (denied / bad VA / negative origin / bad id),
`fffffffe` (string ≥ 48 bytes without NUL).

## String length limit

`HAEUN_GFX_UI_TEXT_MAX = 48` (NUL included → ≤47 payload chars). Longer
guest strings are **rejected** (not silently truncated) on the ECALL path.

## Clipping policy

- Explicit `\n`: new line at origin X
- Auto-wrap at window width (glyph-aligned)
- Horizontal/vertical OOB: `haeun_gfx_wm_put_pixel` returns OOB → skip pixel
- Empty string: success, no writes
- Text never resizes the window

## Window ownership

`deny_win` / `haeun_gfx_wm_owned_by` in `m20_ecall` — foreign PID cannot draw.
Unknown window id denied. pid==0 legacy host tests still allowed.

## Framebuffer pixel verification

`tests/m20_text_tests.c`:

- Exact `'A'` glyph bit pattern vs window backing pixels
- Newline and auto-wrap ink positions
- Transparent space leaves prior fill
- ECALL: foreign window, bad VA, negative origin, oversize, success path

Result: **PASS** (`m20_text_tests ok`)

## SDK examples

| Example | Role | Result |
|---------|------|--------|
| `text_demo` | Hello / digits / newline / colors | PASS |
| `text_layout` | empty, wrap, edge clip | PASS |
| `text_terminal` | prompt + EVENT_POLL / draw_char | PASS |

Runner: `tests/m20_text_process_tests.py`

## Regression

M12 desktop, M19 SDK, M20 process / input / IPC unit+process: **PASS**

## Unimplemented / limits

- No Hangul or proportional fonts
- No dedicated `WIN_DRAW_CHAR` ECALL (libapp uses ECALL 28)
- No scrollback terminal buffer
- Embedded `.word` string literals must not collide with BEQ/BNE/JAL opcodes
  (verifier scans all `.text` words)
- M20.4 window event API not started

## Key files

- `src/gfx_font.c`, `src/gfx_ui.c`, `include/haeun/gfx_ui.h`
- `src/m20_ecall.c`, `userland/libapp.s`
- `sdk/examples/text_{demo,layout,terminal}/`
- `docs/gui-text-v1.md`
