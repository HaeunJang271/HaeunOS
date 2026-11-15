# HAEUN security v2 (syscall dispatch gate)

Status: **a host-side syscall dispatch gate that refuses unknown numbers, validates EXEC images, and checks user VAs (flat window or MMU U-bit) is implemented and tested**. Guest ASM handlers in `kernel/process` are not yet rewritten to call this C gate; the policy must not be bypassed when wired.

## Milestone scope

| Component | Status |
|---|---|
| `haeun_sec_dispatch` policy engine | Done |
| EXEC → `haeun_sec_validate_exec` | Done |
| User VA check via range or MMU | Done |
| Logged denials | Done |
| Guest ASM trap path rewrite | Done in v3 (ECALL poison + deny sink) |
| Secure boot implementation | Not started |

## Dispatch rules

1. Always run `haeun_sec_validate_syscall`.
2. `SYS_EXEC` requires a validated HXF image buffer when provided; entry VA must be user-executable.
3. Pointer-bearing args (future buffer syscalls) use `haeun_sec_check_user_va`.
4. Denials never silently succeed.

## Tests

`tests/sec_dispatch_tests.c` covers allow YIELD, deny unknown syscall, deny bad EXEC image, and deny user VA without U permission.

## Next security milestone

See [security-v3.md](security-v3.md): ECALL pre-trap gate is wired. Remaining: secure boot, and claiming full process isolation only with guest process image tests under the gated ABI.
