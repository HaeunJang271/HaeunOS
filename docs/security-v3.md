# HAEUN security v3 (ECALL gate → process trap)

Status: **user-mode `ECALL` runs `haeun_sec_dispatch` before trap delivery**. On denial the host poisons `r1` to `0xffffffff` so `kernel/process/proc_trap.s` takes the unknown-number path and returns `-1` without side effects. The architectural trap still enters (ISA v4 / existing trapvec tests keep working).

## Milestone scope

| Component | Status |
|---|---|
| `haeun_sec_on_ecall` / `haeun_sec_ecall_before_trap` | Done |
| Wired from `instruction_executor` ECALL | Done |
| Guest trap unknown path (deny sink) | Done (documented) |
| Poison-on-deny preserves trap delivery | Done |
| Guest C `syscall_init` linked in foundation | Still deferred (`K_ERR_DEFERRED`) |
| Secure boot implementation | Not started |

## Rules

1. Supervisor `ECALL` is not gated (kernel path).
2. User `ECALL` always runs dispatch (number + EXEC VA / MMU U).
3. Denial never silently succeeds: `r1` is poisoned and guest returns `-1`.
4. Failures are logged via `haeun_sec_log`.

## Tests

`tests/sec_ecall_tests.c`: allowed YIELD keeps `r1`, unknown number is poisoned, EXEC without `U` is poisoned, trapvec still entered.

## Next

Secure boot research → implementation, or process↔HFS integration with the gated syscall ABI.
