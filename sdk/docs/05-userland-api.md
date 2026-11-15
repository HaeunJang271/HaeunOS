# Userland API Reference

All symbols live in `lib/libapp.s`. Args in `r2..`; result in `r1`; return via `r15`.

| Symbol | ECALL / notes |
|--------|----------------|
| `app_exit` | EXIT (0); set `r2` = code first |
| `app_getpid` | GETPID (9) |
| `app_yield` | YIELD (10) |
| `app_wait` | WAIT (7) |
| `app_spawn` | APP_SPAWN (26); `r2` = index |
| `app_count` | APP_COUNT (24) |
| `app_name` | APP_NAME (25); `r2`=index `r3`=buf `r4`=cap |
| `app_info` | APP_INFO (38); `r2`=index `r3`=field `r4`=buf `r5`=cap |
| `app_run` | APP_RUN (39); `r2`=AS slot → pid/state/win |
| `app_ctrl` | APP_CTRL (40); `r2`=op (0 focus / 1 close) `r3`=child pid |
| `app_putc` | Console `WRITE` (ECALL 1, fd=1); `r2` = byte. Do not use UART MMIO from user apps. |

Window helpers (`app_win_*`, `app_compose`), HFS (`app_open`/`read`/`write`/`close`), and M20 helpers (`app_event_poll`, `app_draw_text`, `app_ipc_*`, `app_svc_reg` / `app_svc_lookup`) are also in `libapp.s`. HOBJ symbol names are limited to 15 characters.

Register apps with the host (`--app path.hxf` or `--apps` builtins). Metadata (`haeun_app_set_meta`) is host-side; guests read it via `app_info`. See `docs/m20-app-launcher-report.md`.

M20.6 process services use ECALL 41–46 (`userland/libsm.s`: `app_sm_*`). IPC name registry remains ECALL 33–34 (`app_svc_reg` / `app_svc_lookup`). See `docs/m20-service-manager-report.md`.

M20.7 HFS assists: ECALL 47–50 (`userland/libfs.s`: `app_fs_readdir` / `unlink` / `stat` / `seek`). Paths remain root basenames. See `docs/m20-file-manager-report.md`.
