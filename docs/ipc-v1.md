# IPC v1 (M20.2)

Host-side message endpoints — **separate** from M20.1 input event queues.

## Addressing

- Receiver creates an endpoint (`IPC_CREATE`) owned by its PID.
- Optional `SVC_REGISTER` / `SVC_LOOKUP` maps a name → epid.
- Any live process may `IPC_SEND` to an existing endpoint (capability model later).
- Only the owner may `RECV` / `CLOSE` / `AVAILABLE` / `PEEK`.

## Message (host)

| Field | Source |
|-------|--------|
| `data[0..len)` | Guest payload (1..32 bytes) |
| `type` | Kernel copies `data[0]` |
| `seq` | Kernel per-endpoint counter |
| `sender_pid` | Kernel current PID (never trusted from guest) |
| `receiver_pid` | Endpoint owner PID |

## Queue

- Depth **8** (`HAEUN_IPC_QUEUE`); max **8** endpoints
- Payload max **32** bytes
- Overflow: **reject** send (`-2` / `0xfffffffd`), increment `drops` — do not drop oldest
- Empty recv → length `0` (not an error)

## ECALL ABI

| ECALL | # | Args / returns |
|-------|---|----------------|
| IPC_CREATE | 29 | → epid |
| IPC_SEND | 30 | r2=epid r3=buf VA r4=len → 0 / `ffffffff` / `fffffffe`(len) / `fffffffd`(full) |
| IPC_RECV | 31 | r2=epid r3=buf VA r4=cap → len; r2=sender (non-blocking) |
| IPC_CLOSE | 32 | r2=epid |
| SVC_REGISTER | 33 | r2=name VA r3=epid |
| SVC_LOOKUP | 34 | r2=name VA → epid |
| IPC_AVAILABLE | 35 | r2=epid → count |
| IPC_PEEK | 36 | r2=epid → r1=len (0 empty); r2=sender r3=type r4=seq |

ECALL 29–34 ABI unchanged from M20.0; 35–36 are additive.

## Lifecycle

EXIT / fault → `haeun_ipc_reap(pid)` clears endpoints and queued messages.

## libapp

`app_ipc_create/send/recv/close`, `app_ipc_avail`, `app_ipc_peek`, `app_ipc_sender`.

Blocking receive is **not** implemented (cooperative `YIELD` + poll instead).
