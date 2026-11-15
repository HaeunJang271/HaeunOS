# Guest GUI syscalls v1

Status: **user `ECALL` numbers 18–23 for window create/destroy/fill/pixel/raise and compose are completed by a host assist over the existing WM/compositor**. Desktop chrome and GUI apps as full userland processes remain future work; Secure boot is separate.

## Syscall ABI

| # | Name | Args | Result |
|---:|---|---|---|
| 18 | `WIN_CREATE` | `r2` x, `r3` y, `r4` w, `r5` h, `r6` backing PA | id / `-1` |
| 19 | `WIN_DESTROY` | `r2` id | `0` / `-1` |
| 20 | `WIN_FILL` | `r2` id, `r3` rgba | `0` / `-1` |
| 21 | `WIN_PIXEL` | `r2` id, `r3` x, `r4` y, `r5` rgba | `0` / `-1` |
| 22 | `WIN_RAISE` | `r2` id | `0` / `-1` |
| 23 | `GFX_COMPOSE` | `r2` desktop rgba | `0` / `-1` |

Framebuffer enable stays a host/setup step (`haeun_gfx_fb_enable`) before installing the assist.

## Wiring

`haeun_vm_set_ecall_io(vm, haeun_gfx_ecall_io, &ctx)` with `ctx.gfx` / `ctx.wm`.

## Tests

`tests/gfx_ecall_tests.c`: create→fill→compose pixel check; YIELD still traps.

## Not claimed

- Guest mouse/keyboard → click raise via syscall
- Desktop strip registration syscalls
- Userland GUI toolkit bindings
