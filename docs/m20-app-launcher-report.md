# M20.5 App Launcher & Process Manager — Report

## Summary

GUI launcher (`userland/gui_launch.s` → `process_launch.hxf`) lists the
host registry, spawns via existing `APP_SPAWN` (26), queries runs via additive
`APP_RUN` (39), and parent-focus / soft-close via `APP_CTRL` (40). Metadata is
exposed through `APP_INFO` (38) without changing `APP_NAME` (25).

## Application Registry

| Field | Storage | Guest access |
|-------|---------|--------------|
| id | registry index 0..count-1 | `APP_COUNT` / spawn index |
| display name | `haeun_app_entry.name` (≤15) | `APP_NAME` / `APP_INFO` field 0 |
| description | `desc` (≤23) | `APP_INFO` field 1 |
| version | `version` (≤7) | field 2 |
| category | `category` (≤7) | field 3 |
| flags | `flags` (`HAEUN_APP_FLAG_SINGLE`) | field 4 → `r2` |
| executable | host HXF pointer (not guest-visible) | spawn by index only |

Registration remains host-side (`haeun_app_register` / `--apps` / `--app`).
Duplicate names and invalid HXF are rejected. v1 has no package installer.

## Launch flow

1. Guest reads `APP_COUNT` / `APP_NAME` / `APP_INFO`.
2. `APP_SPAWN(index)` validates HXF, allocates AS slot 1–2, READY PCB.
3. Failure → `r1 = -1` (ABI unchanged; host distinguishes INVALID/NOSPC/BUSY).
4. Child EXIT/FAULT → mux `haeun_app_reap` (windows + AS scrub); parent `WAIT`.

## Running apps

`APP_RUN(slot)` for AS slots 0..2 (slot 0 is launcher AS; apps use 1–2):

- `r1=0`, `r2=pid`, `r3=app_index`, `r4=state|(exit_code<<16)`, `r5=win_id` or `-1`

PCB state is read-only from guest RAM; no PCB mutation from launcher ECALLs.

## PID / Window link

- `owner_pid` on `WIN_CREATE` (M14).
- `APP_RUN` scans WM for first window of pid.
- `APP_CTRL FOCUS`: parent-only raise+focus + focus events.
- `APP_CTRL CLOSE`: parent-only `WINDOW_CLOSE_REQUEST` (no destroy).

## Duplicate policy

Default: multiple instances allowed until AS slots exhausted (`NOSPC`).
`HAEUN_APP_FLAG_SINGLE` → second spawn returns `HAEUN_APP_BUSY` (`-1` to guest).

## Error handling

| Condition | Host | Guest |
|-----------|------|-------|
| bad index / unregistered | BAD_ARGUMENT | `-1` |
| bad HXF | INVALID | `-1` |
| no AS slot | NOSPC | `-1` |
| single-instance busy | BUSY | `-1` |
| foreign CTRL | INVALID | `-1` |
| child fault | M17 ZOMBIE + reap | launcher survives |

## Userland API (`libapp.s`)

`app_count`, `app_name`, `app_spawn`, `app_info`, `app_run`, `app_ctrl`

## SDK registration

1. Build HXF (`haeun.py build` or `build_app_hxf.py`).
2. Run with `process_launch.hxf` or `process_apps.hxf` and `--app path.hxf`.
3. Optional host meta: `haeun_app_set_meta` (builtins set in `main.c`).

## Tests

| Suite | Result |
|-------|--------|
| `m20_launcher_tests` (meta, spawn/run/ctrl, single, ECALL) | PASS |
| `m20_launcher_process_tests` (`process_launch.hxf --apps`) | PASS |
| M14 `process_apps` / M15 / M19 / M20.1–4 process samples | PASS |

## Manual viewer

```text
build/haeun-vm.exe build/process_launch.hxf --apps --viewer
```

Smoke path paints app list, spawns calc, focuses/close-requests, dual-spawns
calc+edit when possible, paints OK, exits. Interactive dwell is limited by the
finite smoke script (not an infinite shell).

## Limits

- Max 4 registry entries; 2 concurrent app AS slots (+ launcher).
- No dynamic linker / package install (out of scope).
- Guest SPAWN still collapses all failures to `-1`.
- `process_apps.hxf` keeps the M14 batch `launcher.s` for regression.
