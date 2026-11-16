# HAEUN process isolation & security hardening (M15)

Status: **memory AS isolation, PTE/TLB scrub on reap, window/fd/socket ownership,
and automated isolation tests are verified**. Intentional M14 v1 limits are unchanged.

## Phase 1 audit (source-backed)

| Area | Status | Evidence |
|---|---|---|
| AS slot alloc (1–2) | IMPLEMENTED | `haeun_app_spawn` in `src/app_env.c` |
| HXF validate + code PA copy | IMPLEMENTED | `extract_user`, `memset`/`memcpy` to slot PA |
| L0 PTE remap + TLB flush on spawn | IMPLEMENTED | PTE write + `haeun_tlb_flush` |
| Code PTE non-writable | IMPLEMENTED | flags `0x1b` = V\|R\|X\|U (no `PTE_W`) |
| Per-slot stack pages | IMPLEMENTED | distinct PA `0x113000`/`0x114000`/`0x115000` |
| Cross-AS VA deny | IMPLEMENTED | separate L0; MMU translate tests |
| EXIT code/PTE scrub | IMPLEMENTED | `scrub_slot` in `haeun_app_reap` |
| Window `owner_pid` | IMPLEMENTED | `gfx_wm.c`, `gfx_ecall.c` deny foreign |
| EXIT window reclaim | IMPLEMENTED | `haeun_app_destroy_windows` via mux |
| Socket `owner_pid` | IMPLEMENTED | set on SOCKET/ACCEPT; checked on BIND/SENDTO/RECVFROM/LISTEN/ACCEPT/CLOSE; EXIT reclaim |
| HFS fd ownership | IMPLEMENTED | `proc_fd.owner_pid`, close-by-owner |
| Failed SPAWN before mutate | IMPLEMENTED | validate/extract before PCB/code write |
| Shared KBD FIFO | PARTIAL (by design) | `board_extra.c` `kbd_read` drains globally |
| WIN_CREATE raw PA | PARTIAL (ABI kept) | guest PA validated (align/RAM/FB/overlap; process pool `[0x18000,0x100000)`) |
| GFX_COMPOSE global | PARTIAL (ABI) | any process may compose |

## Hardening applied (M15)

1. `haeun_app_reap`: wipe code PA, wipe stack page, invalidate user-code PTE, TLB flush.
2. `haeun_proc_fd` ownership + EXIT close-by-owner (with socket/window).
3. Isolation automated tests in `tests/m15_isolation_tests.c`.

## Shared KBD FIFO (M15.5)

- Bytes are consumed in FIFO order by whoever reads `HAEUN_KBD_BASE`.
- No process filter; a background process can steal keys from the focused app.
- Exited processes do not run, so they cannot consume input after EXIT.
- **v1 limit**: per-process input queues remain out of scope.

## Tests

```text
make build/m15-isolation-tests.exe && ./build/m15-isolation-tests.exe
make build/m14-app-env-tests.exe && ./build/m14-app-env-tests.exe
python tests/m14_process_apps_tests.py build/haeun-vm.exe
python tests/m13_process_net_tests.py build/haeun-vm.exe
python tests/m9_process_net_tests.py build/haeun-vm.exe
python tests/m8_process_gui_tests.py build/haeun-vm.exe
python tests/m7_guest_gui_tests.py build/haeun-vm.exe
./build/m7-integration-tests.exe
./build/m13-net-backend-tests.exe
```

Coverage: invalid HXF, PTE remap, TLB/AS deny, slot reuse scrub, window deny/cleanup,
fd/socket cleanup, KBD FIFO order, unmapped VA fault, M14/M7–M13 regressions.

Manual-only (not automated): physical keyboard focus UX with live `--viewer`.

## Remaining intentional limits

AS slots ≤3, app ≤4KiB, no HFS EXEC, no package/dynamic link,
default FB **320×180**, `WIN_DRAW_TEXT` / `EVENT_POLL` available to process apps,
shared KBD FIFO retained for legacy demos, APP_SPAWN loads images.
(See [limitations.md](limitations.md), [graphics-api.md](graphics-api.md).)

## Verdict

| Capability | Verdict |
|---|---|
| Memory AS / PTE / TLB scrub | IMPLEMENTED |
| Window owner_pid ACL | IMPLEMENTED |
| FD/socket EXIT cleanup | IMPLEMENTED |
| Slot reuse safety | IMPLEMENTED |
| Shared KBD FIFO isolation | PARTIAL (v1 design) |
| Per-process input queues | NOT IMPLEMENTED (intentional) |
