# Intentional limitations (current)

These are **design caps**, not open bugs. Synced M22.2 with runtime (`include/haeun/vm_gui_host.h`, process-v1, M20/M21/M22).

| Area | Limit |
|---|---|
| AS slots | Max 3 (init + 2 apps) |
| App code | ≤ 4KiB @ `0x00108000` |
| Framebuffer | **320×180** @ `0x00200000` (default `--gui` / desktop) |
| Windows | Max 8 |
| Sockets | Max 4 |
| HFS FDs | Max 4 (global table) |
| Input | Per-pid `EVENT_POLL` queues for process apps; shared KBD FIFO retained for legacy demos |
| Text | ASCII 8×8 only (`WIN_DRAW_TEXT`); no Hangul |
| Packaging | No installers / dynamic linking / HFS EXEC |
| Faults | No user signals/restart; kernel fault → halt |
| Viewer | Win32 GDI; non-Windows stub unsupported |
| External DNS | Opt-in via `HAEUN_NET_EXTERNAL=1` |

### Supported vs partially tested (desktop)

| Feature | Status |
|---------|--------|
| 320×180 shell, three tiles, keys 1–3, Esc, `--persist` | **Supported**; covered by `m21_*` / `m22_1_*` process tests |
| Calculator / Editor keyboard interaction | **Supported**; guest scripted coverage in `m22_3_interactive_app_tests.py` (not physical HID) |
| Physical HID (real keyboard/mouse beyond Win32 viewer path) | **Not claimed** as separately validated |

Security/isolation docs: [security-isolation-v1.md](security-isolation-v1.md), [fault-handling-v1.md](fault-handling-v1.md).  
Graphics surface: [graphics-api.md](graphics-api.md).
