# M20.6 Service Manager — Report

## Audit summary (Phase 1)

| Question | Finding | v1 impact |
|----------|---------|-----------|
| PCB states | FREE/READY/RUNNING/SLEEP/WAIT/ZOMBIE | Status synced from PCB |
| APP_SPAWN / APP_CTRL | Spawn slots 1–2; parent FOCUS/CLOSE | Services spawn via same path |
| Exit code | PCB+16; EXIT arg / fault cause | Preserved in `last_exit` |
| Events / IPC | Per-PID evt; IPC separate; SVC 33–34 = name→epid | Stop = push CLOSE event |
| Background | No dedicated daemon class | Windowless apps = “service” |
| ECALL map | 0–40 used | New **41–46** |
| Windowless | Yes (no WIN_CREATE) | `app_svc_idle.s` |
| Boot AUTO | Host can start after register | `haeun_sm_autostart` (PARTIAL) |

Existing **SVC_REGISTER/LOOKUP (33–34)** remain an IPC name registry and are **not** reassigned.

## Architecture

```
Service Registry (host haeun_sm_hub)
  → app_index → haeun_app_env HXF
SERVICE_START → haeun_app_spawn → PID + PCB READY/RUNNING
SERVICE_STOP  → evt CLOSE (+ optional window CLOSE); state STOPPING
EXIT/FAULT mux → haeun_sm_on_pid_exit → STOPPED/FAILED + pending_restart
SERVICE_STATUS → PCB sync + try deferred restart
```

Services are **user processes**, not a separate kernel mode. Isolation = M15 AS/PID ownership.

## Classification

| Criterion | Status |
|-----------|--------|
| Registry query (COUNT/INFO) | **IMPLEMENTED** |
| Start as real user process | **IMPLEMENTED** |
| PID ↔ service link | **IMPLEMENTED** |
| Normal vs abnormal exit | **IMPLEMENTED** (STOPPING→STOPPED vs FAILED; fault flag) |
| Resource reclaim on exit | **IMPLEMENTED** (existing mux reap) |
| Restart NEVER / ON_FAILURE / ALWAYS | **IMPLEMENTED** (deferred until AS/PCB free) |
| Restart delay / interval timer | **NOT IMPLEMENTED** (no host timer added) |
| GUI launcher service manage | **IMPLEMENTED** (smoke: start/status/stop + “SVC” text) |
| Infinite interactive service UI | **PARTIAL** (finite smoke in `gui_launch.s`) |
| Service fault kills launcher | **IMPLEMENTED** (isolated; launcher continues) |
| True background/out-of-scheduler | **NOT IMPLEMENTED** (same RR as apps) |
| Boot AUTO start without guest | **PARTIAL** (`haeun_sm_autostart` API; not forced in `main`) |

## Registry fields

`name`, `desc`, `version`, `app_index` (executable_id), `startup_type` (MANUAL/AUTO),
`restart_policy` (NEVER/ON_FAIL/ALWAYS), runtime: `state`, `pid`, `run_count`,
`last_exit`, `fail_count`, `restart_count`, `max_restarts` (3), `pending_restart`,
`generation`.

## API (ECALL 41–46)

| # | Name | Notes |
|---|------|-------|
| 41 | SERVICE_COUNT | → count |
| 42 | SERVICE_INFO | field 0–2 strings; 3 flags pack |
| 43 | SERVICE_START | → pid or -1 |
| 44 | SERVICE_STOP | cooperative CLOSE event |
| 45 | SERVICE_STATUS | state, pid, exit/runs, fails/restarts |
| 46 | SERVICE_RESTART | stop request then start (slot may need WAIT) |

Guest wrappers: `userland/libsm.s`.

## State transitions

```
STOPPED --start--> STARTING --PCB live--> RUNNING
RUNNING --stop--> STOPPING --clean EXIT--> STOPPED
RUNNING --unexpected EXIT/FAULT--> FAILED
FAILED --pending_restart + STATUS--> STARTING/RUNNING (policy)
```

Fake RUNNING without PCB live is rejected at start (PCB check).

## Restart policy

- **NEVER**: stay FAILED/STOPPED.
- **ON_FAILURE**: pending restart on fault or nonzero exit.
- **ALWAYS**: pending restart on unexpected death (not cooperative STOP).
- Cap: `max_restarts` (3). Applied on `SERVICE_STATUS` when slot free (no delay timer).

## Security

- Invalid service id → -1.
- Duplicate service name / bad app_index rejected at register.
- STOP uses parent check via APP_CTRL when applicable; always pushes per-PID event.
- Spawn failures: NOSPC/INVALID; no PCB poke.
- PID reuse: `pid` cleared on exit; `generation` bumped on start.

## Tests

| Suite | Result |
|-------|--------|
| `m20_service_tests` | PASS |
| `m20_launcher_process_tests` (svc idle + GUI) | PASS |
| `sec_dispatch` (SYS_MAX+1) | PASS |
| M14 / M16 / M17 / M20.4 samples | PASS |

```text
./build/m20-service-tests.exe
python tests/m20_launcher_process_tests.py build/haeun-vm.exe
```

## Manual viewer

```text
build/haeun-vm.exe build/process_launch.hxf --apps --viewer
```

Expect app list, calc/edit, then service start (“SVC”) and OK.

## Limits

- Max 4 services; share 2 AS app slots with normal apps.
- Cooperative stop only (no forced PCB kill).
- No restart backoff interval.
- IPC name registry (33–34) ≠ process Service Manager.
