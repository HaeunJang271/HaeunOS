# HAEUN graphics v1 (milestone 1)

Status: **framebuffer driver, pixel renderer, and 8×8 font renderer are implemented and tested**. See [graphics-v2.md](graphics-v2.md) for the window manager and compositor.

## Milestone scope

| Component | Status |
|---|---|
| Framebuffer MMIO (`0x10004000`) | Board device (prior) |
| Framebuffer driver (`haeun_gfx_fb_*`) | Done — configures WIDTH/HEIGHT/BASE/CONTROL, checks STATUS |
| Pixel renderer | Done — clear, put, fill rect, with bounds checks |
| Font renderer | Done — 8×8 glyphs for printable ASCII |
| Window manager / compositor | Done in v2 |
| GUI toolkit | Done in v3 |

| Desktop / user GUI apps | Not started |

## Pixel format

RGBA8888 little-endian in guest RAM at the programmed BASE. Pitch is `width * 4` bytes. Drawing APIs write guest RAM only after the framebuffer is enabled and STATUS reports ready.

## Driver contract

1. Program WIDTH, HEIGHT, BASE (guest PA of the pixel buffer).
2. Set CONTROL.enable = 1.
3. STATUS must read 1 before drawing; otherwise draws are rejected.
4. All coordinates are clipped or rejected; no unchecked host pointer math from guest addresses beyond the validated buffer range.

## Tests

`tests/gfx_tests.c` covers enable failure paths, pixel writes, rectangle fills, and string rendering against the in-RAM framebuffer.
