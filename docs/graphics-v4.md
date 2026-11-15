# HAEUN graphics v4 (minimal desktop)

Status: **root wallpaper window, bottom task strip, client registration, present/compose, and strip click→raise are implemented and tested**. GUI apps still do not run as user processes; this layer is host-orchestrated over guest-RAM backings.

## Milestone scope

| Component | Status |
|---|---|
| Root / wallpaper window | Done |
| Task strip window | Done |
| Register client + strip button | Done |
| Present (paint + compose) | Done |
| Strip click raises client | Done |
| User-process GUI syscalls | Done in gfx-syscall-v1 (ECALL assist) |

## Model

`haeun_gfx_desktop` owns two WM windows (root + strip) whose backings are guest physical addresses that must not overlap the primary framebuffer or each other. Client windows are created by the caller and registered with a title; the strip paints a button per client. `haeun_gfx_desktop_present` redraws chrome and calls `haeun_gfx_compose`. Screen-space clicks on the strip raise the matching client.

## Tests

`tests/gfx_desk_tests.c` builds a desktop, registers a client, presents, checks wallpaper/strip/client pixels, and verifies raise-on-click.

## Next milestone

See [gfx-syscall-v1.md](gfx-syscall-v1.md) for guest window ECALL wiring.
