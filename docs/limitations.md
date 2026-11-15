# Intentional Release 1.0 limitations

These are **v1 design caps**, not open bugs:

| Area | Limit |
|---|---|
| AS slots | Max 3 (init + 2 apps) |
| App code | ≤ 4KiB @ `0x00108000` |
| Framebuffer | 64×32 |
| Windows | Max 8 |
| Sockets | Max 4 |
| HFS FDs | Max 4 (global table) |
| Input | Shared KBD FIFO; no per-process queues |
| GUI | No WIN_DRAW_TEXT / POLL_EVENT |
| Packaging | No installers / dynamic linking / HFS EXEC |
| Faults | No user signals/restart; kernel fault → halt |
| Viewer | Win32 GDI; non-Windows stub unsupported |
| External DNS | Opt-in via `HAEUN_NET_EXTERNAL=1` |

Security/isolation docs: [security-isolation-v1.md](security-isolation-v1.md), [fault-handling-v1.md](fault-handling-v1.md).
