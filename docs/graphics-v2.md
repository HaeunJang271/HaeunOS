# HAEUN graphics v2 (window manager + compositor)

Status: **window manager and compositor are implemented and tested**. See [graphics-v3.md](graphics-v3.md) for the GUI toolkit. Desktop environment and user-process GUI syscalls remain deferred.

## Milestone scope

| Component | Status |
|---|---|
| Per-window guest-RAM backing | Done |
| Create / destroy / move / resize registry | Done |
| Z-order (raise / lower) | Done |
| Compositor → primary framebuffer | Done |
| GUI toolkit widgets | Done in v3 |
| Desktop shell / user GUI processes | Not started |

## Model

Each window owns a rectangular RGBA8888 buffer at a guest physical address. The compositor never invents pixels on the host: it copies from validated guest backing into the primary framebuffer programmed by Graphics M1 (`haeun_gfx`).

1. Register a window with `base`, `w`, `h` that fit inside guest RAM.
2. Draw into the backing (fill / put pixel APIs).
3. Call `haeun_gfx_compose` to clear the FB and blit visible windows bottom→top by z-order.

Invalid ranges, unready FB, or unknown window ids return errors; no silent bypass.

## Tests

`tests/gfx_wm_tests.c` covers registration bounds, z-order occlusion, and composed pixels in the primary FB.

## Next graphics milestone

Completed in v3 (toolkit). Next: minimal desktop environment, then user-process GUI syscalls.
