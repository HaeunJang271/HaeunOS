# HAEUN application environment v1 (M14)

Status: **standalone HXF apps can be registered, spawned into independent
process-v1 slots, own WM windows by PID, and exit with resource cleanup**.

## Phase 1 audit (pre-M14)

| Area | Status | Notes |
|---|---|---|
| HXF load/validate | IMPLEMENTED | `hxf.c`, `haeun_sec_validate_exec` |
| PCB / FORK / EXIT / RR | IMPLEMENTED | `docs/process-v1.md`, NPROC=4, 3 AS slots |
| EXEC | PARTIAL | Entry replace only; no image load |
| HFS + path OPEN | IMPLEMENTED | Host `proc_ecall_io` assist |
| WM / GFX ECALL | IMPLEMENTED | No PID ownership pre-M14 |
| Desktop chrome | IMPLEMENTED | M12; no app launcher |
| Multi-app launch | NOT IMPLEMENTED | Single user_init per image |

## Architecture

```
Launcher (PCB0 @ 0x108000)
  APP_COUNT / APP_SPAWN ECALL
    → haeun_app_env: validate HXF → copy to slot PA → remap PTE → READY PCB
Child apps (slots 1–2)
  WIN_* with owner_pid
  EXIT → socket/window reap + ZOMBIE → parent WAIT
```

HXF format unchanged. App images are one-segment user programs at VA `0x00108000`.
Per-slot code PA: slot1 `0x10C000`, slot2 `0x10D000` (VA still `0x108000`).

## ECALLs (additive)

| # | Name | Args |
|---:|---|---|
| 24 | APP_COUNT | → count |
| 25 | APP_NAME | index, buf VA, cap |
| 26 | APP_SPAWN | index → pid |
| 38 | APP_INFO | index, field, buf, cap (M20.5) |
| 39 | APP_RUN | AS slot → pid / app_index / state / win |
| 40 | APP_CTRL | op (focus/close), child pid (parent-only) |

## Window ownership

`haeun_gfx_window.owner_pid` set on WIN_CREATE from PCB pid. Mutating ECALLs
deny foreign owners. EXIT destroys owned windows.

## Userland

- `userland/libapp.s` — shared wrappers
- `userland/launcher.s` — M14 batch spawn/wait (regression)
- `userland/gui_launch.s` — M20.5 GUI list + APP_RUN/CTRL smoke
- `userland/app_calc.s` / `app_edit.s` / `app_files.s`

## CLI

`haeun-vm build/process_apps.hxf --apps` (loads `build/app_*.hxf`).  
`haeun-vm build/process_launch.hxf --apps` (GUI launcher image).

## Limits

- Max 3 address spaces (`HAEUN_APP_SLOTS`: launcher + 2 concurrent apps)
- Registry capacity `HAEUN_APP_MAX` = 4 (CLI `--app` capped to the same value)
- `FORK` only allocates PCB slots 0..2 (no AS for a 4th PCB)
- App code ≤ 4 KiB
- Guest `WIN_CREATE` backing PA must lie in `[0x18000, 0x100000)` when pid ≠ 0
- No dynamic linking / package install
- Shared KBD FIFO (no per-process input queue)
- Calculator uses programmed operands (not interactive keypad UI)
