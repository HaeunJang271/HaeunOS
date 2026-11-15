# M20.2 Inter-Process Communication — Report

## Summary

HAEUN-OS IPC v1 delivers fixed-size messages through **per-endpoint queues**
owned by a creating PID. Input event queues (M20.1) remain separate. Existing
ECALL 29–34 ABI is preserved; `IPC_AVAILABLE` (35) and `IPC_PEEK` (36) are added.

## Message data structure

Host `haeun_ipc_msg` (not shared with `haeun_evt`):

| Field | Set by | Notes |
|-------|--------|-------|
| `data[]` / `len` | Guest payload copy | 1..32 bytes |
| `type` | Kernel | `data[0]` |
| `seq` | Kernel | Per-endpoint monotonic |
| `sender_pid` | Kernel | Current process PID only |
| `receiver_pid` | Kernel | Endpoint owner PID |

Guest apps may also place structured fields in the payload (word-aligned type/seq
used by pingpong demos).

## Queue capacity and overflow

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Endpoints | 8 | Matches small NPROC / APP slot scale |
| Queue depth | 8 | Headroom for short bursts without large RAM |
| Payload max | 32 B | Fits stack copies; AS-friendly |
| Overflow | **Reject** (`-2`), `drops++` | Do not silently discard IPC (unlike input evt drop-oldest) |

## ECALL numbers and ABI

| Call | # | ABI |
|------|---|-----|
| IPC_CREATE | 29 | → epid |
| IPC_SEND | 30 | epid, buf VA, len → 0 / err |
| IPC_RECV | 31 | epid, buf VA, cap → len; r2=sender |
| IPC_CLOSE | 32 | epid |
| SVC_REGISTER / LOOKUP | 33 / 34 | name discovery |
| IPC_AVAILABLE | 35 | epid → count |
| IPC_PEEK | 36 | epid → len, sender, type, seq |

Errors: `ffffffff` bad/denied, `fffffffe` bad length, `fffffffd` queue full.
User buffers checked with `haeun_sec_check_user_va` + MMU-safe copy.

libapp: `app_ipc_create/send/recv/close/avail/peek/sender`.

## Permission model

- Create → endpoint owned by caller PID.
- Recv / close / available / peek → **owner only**.
- Send → any process with a valid epid (active endpoint).
- Closed / reaped endpoints reject send (dead-owner equivalent).
- Sender PID is never taken from guest registers as authority.
- Future: capability tokens can wrap epid without ABI break.

## PID validation

- `sender_pid` from `haeun_guest_pcb_current_pid` in the ECALL assist.
- Endpoint must be `used`; owner_pid > 0.
- Foreign recv/peek/available logged and denied.
- Discovery of peers via `SVC_LOOKUP` (name → epid), not raw forgeable PIDs.

## Lifecycle and reclaim

- SPAWN: no IPC state until `IPC_CREATE`.
- EXIT / M17 fault: `haeun_ipc_reap` (+ `svc_reap`) via `ecall_mux`.
- Reap zeroes the slot so PID reuse cannot observe old messages.
- Input `haeun_evt_*` paths untouched.

## Test applications

| Role | Sources |
|------|---------|
| ipc_receiver / ipc_sender | `app_ipc_rx.s`, `app_ipc_tx.s` |
| ipc_pingpong | `app_ipc_sv.s` + `app_ipc_cl.s` |
| Concurrent runner | `ipc_dual.s` (SPAWN both, WAIT×2) |

## Automated test results

| Suite | Result |
|-------|--------|
| `m20_ipc_tests` (FIFO, isolation, overflow, VA, peek, pingpong seq, reap) | PASS |
| `m20_ipc_process_tests` (dual-app send/recv + pingpong) | PASS |
| `m20_unit_tests` | PASS |
| M20.1 input unit + process | PASS |
| M10 / M14 / M17 / M19 / M20.0.1 / M20.0.2 / M20 process | PASS |

## Unimplemented / limits

- No blocking `recv` (use `YIELD` + poll / available).
- No shared-memory IPC; no capability tokens yet.
- Send addresses endpoints (not raw PID); use SVC name lookup for discovery.
- No cross-node / network IPC.
- M20.3 text rendering and M20.4 window events not in this milestone.

## Key files

- `include/haeun/ipc.h`, `src/ipc.c`
- `src/m20_ecall.c`, `include/haeun/sec.h`, `userland/libapp.s`
- `docs/ipc-v1.md`
