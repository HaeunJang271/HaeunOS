# GUI Text v1 (M20.3)

## Font

- Format: static 8×8 bitmaps in `gfx_font.c` (`FONT8[95][8]`)
- Coverage: printable ASCII 32–126
- Bit7 of each row = leftmost pixel
- Unsupported / non-printable (except `\n`/`\r` in draw_text) → `'?'` glyph
- `bg == 0` → transparent background (fg pixels only)

## Layout

- Glyph advance: 8 px
- Explicit `\n`: reset X to origin, Y += 8
- Auto-wrap when next glyph would exceed window width
- Vertical overflow: stop / clip via `put_pixel` OOB
- Empty string: success, no pixels
- Window size is never changed by text output

## ECALL

`WIN_DRAW_TEXT` (**28**), ABI unchanged:

| Reg | Meaning |
|-----|---------|
| r2 | window id |
| r3 | x (window-local, ≥0) |
| r4 | y (window-local, ≥0) |
| r5 | fg RGBA |
| r6 | bg RGBA (`0` = transparent) |
| r7 | NUL-terminated string VA |

Returns: `0` ok, `ffffffff` denied/bad, `fffffffe` string longer than 47 chars (+NUL).

Ownership enforced for pid≠0. Renders into **window backing**, not raw FB.

## libapp

- `app_draw_text` — full string
- `app_draw_char` — single glyph via the same ECALL

UART `app_putc` remains separate (console).

## Limits

- Max string: `HAEUN_GFX_UI_TEXT_MAX` = 48 (including NUL)
- No Hangul / proportional fonts
- Default FB 64×32 ≈ 8×4 glyphs of 8×8
