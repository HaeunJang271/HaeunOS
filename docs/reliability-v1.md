# HAEUN system reliability & boot integrity (M16)

Status: **host boot failure checks, fault-input rejection, resource exhaustion
returns, SPAWN/EXIT loops, GUI/net lifecycle, and M7–M15 regressions are verified**.
Guest page-fault process kill remains a known v1 limitation.

## Boot sequence

1. Host `main` loads HXF or `--firmware` HFW (+ optional disk / root-hash).
2. RAM allocate → image load / HBI encode → `haeun_vm_init_version`.
3. Optional Boot ROM protect, block attach, UART sink.
4. `--apps` / `--gui` attach (FB+WM[+chrome][+HFS+APP_ENV]) installs ECALL mux.
5. `--net` attach (NIC+socks); if GUI/apps also active, **sock_ctx is merged**
   back into the GUI mux (M16 fix for mux overwrite).
6. Optional input / viewer, then run loop; shutdown cleans net backend.

Guest firmware (`firmware/boot.s`) validates a constrained HXF from disk, writes
HBI, transfers to kernel. `kernel/process/proc_boot.s` builds AS slots + PCB0,
enters user init. `APP_SPAWN` loads apps into free AS slots 1–2.

## Initialization dependencies

| Stage | Depends on | Failure behavior |
|---|---|---|
| VM init | RAM + entry | stderr + nonzero exit |
| GUI/apps attach | VM | attach fails → exit before run |
| Net attach | VM | attach fails → exit |
| Combined apps+net | GUI mux first | mux merge after net (M16) |
| APP_SPAWN | free AS + valid HXF | `HAEUN_APP_NOSPC` / `INVALID` |

Partial host attach mid-failure leaves stack objects; process exits without run.

## Fault handling policy

| Input | Host/assist behavior | Guest process kill? |
|---|---|---|
| Bad HXF / entry / size | reject; no mutate | N/A (register/spawn fail) |
| Bad ECALL number | sec poison / trap `-1` | continues |
| Bad window/fd/socket | `0xFFFFFFFF` / error code | continues |
| Unmapped VA (MMU) | page-fault cause raised | **IMPLEMENTED (M17)**: user → ZOMBIE + reap; see [fault-handling-v1.md](fault-handling-v1.md) |
| Unsupported opcode | trap / host stop | depends on ISA path |

Kernel-wide halt vs process-local deny: assists deny without corrupting other PCBs.
True process abort on page fault is **not implemented** in process-v1 trap.

## Resource exhaustion

| Resource | Limit | Exhaustion | Reclaim |
|---|---|---|---|
| AS slots (spawn) | 2 free of 3 | `HAEUN_APP_NOSPC` | reap + PCB FREE |
| Windows | 8 | create fails | destroy / EXIT |
| Sockets | 4 | `HAEUN_NET_NOSPACE` | close / EXIT |
| HFS FDs | 4 (global table) | `HAEUN_HFS_NOSPC` | close / EXIT |

## Scheduler stability (tested)

- 32× SPAWN→reap→slot reuse (`m16_reliability_tests`).
- M14 process apps end-to-end spawn of registered apps.
- Round-robin READY/RUNNING in `proc_sched.s` (no BLOCKED I/O wait assumed).

## GUI / network lifecycle

- Window create/destroy + owner reclaim loops.
- UDP socket open/close + owner reclaim loops.
- FB OOB pixel rejected.
- M13 backend unit tests; external DNS only when `HAEUN_NET_EXTERNAL=1`.

## Tests

```text
make build/m16-reliability-tests.exe && ./build/m16-reliability-tests.exe
python tests/m16_repeat_tests.py build/haeun-vm.exe
# plus M7–M15 targets used by `make test`
```

## Known limitations

- Shared KBD FIFO (no per-process input).
- Guest page-fault → dedicated EXIT not implemented.
- Window exhaustion error not distinct from bad args.
- Firmware failure prints generic `E`; no structured boot telemetry.
- Intentional v1 caps: AS≤3, app≤4KiB, FB 64×32, etc.
