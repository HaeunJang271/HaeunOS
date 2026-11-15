# Graphics API (Release 1.0)

## Host geometry

Default `--gui` / `--apps` framebuffer: **64×32** at PA `0x10000` (`HAEUN_GUI_FB_*`).

## Guest ECALL

| # | Call | Ownership |
|---:|---|---|
| 18 | WIN_CREATE | sets `owner_pid` |
| 19 | WIN_DESTROY | owner only |
| 20 | WIN_FILL | owner only |
| 21 | WIN_PIXEL | owner only |
| 22 | WIN_RAISE | owner only |
| 23 | GFX_COMPOSE | global (any process) |

Not implemented: `WIN_DRAW_TEXT`, `POLL_EVENT`, per-process input queues.  
Shared KBD FIFO at `HAEUN_KBD_BASE`.

Viewer: `--viewer` (Win32 GDI). See [graphics-v1.md](graphics-v1.md)–[v4](graphics-v4.md), [viewer-v1.md](viewer-v1.md), [input-v1.md](input-v1.md).
