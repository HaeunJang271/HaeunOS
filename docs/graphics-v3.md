# HAEUN graphics v3 (GUI toolkit)

Status: **label and button widgets that paint into window guest-RAM backings are implemented and tested**. See [graphics-v4.md](graphics-v4.md) for the minimal desktop. User-process GUI syscalls remain deferred.

## Milestone scope

| Component | Status |
|---|---|
| Label (text on window backing) | Done |
| Button (fill + border + text + hit-test) | Done |
| Desktop shell | Done in v4 |
| GUI apps as user processes | Not started |

## Model

Widgets never invent a separate host framebuffer. They call `haeun_gfx_wm_put_pixel` / fill helpers so every pixel lands in the window’s guest-physical backing. Text uses the shared 8×8 font (`haeun_gfx_font8_rows`). Hit-testing is window-local coordinates; wiring guest mouse/keyboard to user processes is a later milestone.

## Tests

`tests/gfx_ui_tests.c` draws a label and button, composes to the primary FB, checks pixels, and verifies hit / miss.

## Next graphics milestone

Completed in v4 (desktop). Next: user-process GUI syscalls, or Networking M1.
