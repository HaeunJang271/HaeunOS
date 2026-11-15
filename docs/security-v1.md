# HAEUN security v1

Status: **privilege gate, syscall validation, HXF validation, logging, M15 isolation, and M17 user-fault containment are in Release 1.0**. Secure boot measure/verify: [secure-boot-v1.md](secure-boot-v1.md).

## Milestone scope

| Component | Status |
|---|---|
| User / supervisor privilege checks | Done (gate API + tests; ISA v4 already faults CSR in user) |
| Syscall validation (number + caller privilege) | Done |
| Executable validation (HXF magic/ISA/entry) | Done |
| Security failure logging | Done (stderr audit line; never silent deny) |
| File permissions | Prior (HFS); not re-implemented here |
| Memory protection | Prior (MMU/TLB); gate helpers for user-pointer bounds |
| Process isolation hardening | Done for M15 scope — see [security-isolation-v1.md](security-isolation-v1.md) |
| Secure boot implementation | Host measure/verify done (v1); firmware path later |

## Rules

1. Denied operations return `HAEUN_SEC_DENIED` and emit a log line.
2. Syscall validation does not bypass privilege: user may only invoke the allow-listed user syscall numbers.
3. Executable validation rejects truncated/non-HXF images before load paths treat them as trusted.

## Secure boot

Host SHA-256 measure/verify/allow-list: see [secure-boot-v1.md](secure-boot-v1.md). Firmware still does not refuse handoff; direct CLI HXF load without verify remains a non-secure-boot path.

## Tests

`tests/sec_tests.c` covers privilege deny, syscall allow/deny, and HXF reject/accept with logged failures.

## Process isolation (M15)

Memory AS scrub on reap, window/fd/socket ownership, and isolation tests:
[security-isolation-v1.md](security-isolation-v1.md).

## Next security milestone

Guest-side SHA measurement and deeper user-pointer walk checks remain optional follow-ups.
