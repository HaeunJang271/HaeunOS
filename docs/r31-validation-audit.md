# R3.1 Final Audit

## Kernel

Compared to R3.0 baseline hashes in `docs/r31-preimplementation-audit.md`.
Post-R3.1 `git hash-object` matched exactly:

- `proc_sched.s` `fe6a0ef11864a1947e5fc28d3929e86bd0d55233`
- `proc_policy.s` `dee3a76622a8d2acfcb5a0e958b14f74056dbb52`
- `proc_trap.s` `a2c1f917b346bcb42ed884d102065cf006091a89`
- `proc_telem.s` `8349bdacf7019e239941fc4a6f39c40088b637c4`

`make clean` / `make all` / `make test` exit 0 (executed).

## Host additions

`src/pval_*.c`, `include/haeun/pval_*.h`, CLI flags in `src/main.c`, tests `tests/r31_pval_*.c`.

## Guarantees

- No guest RAM writes from validator.
- No guest instructions during validate.
- No runtime policy switching.
- No PCB/telem/ABI/timer changes.

## Honest overall status

Typical halted workloads report overall **UNKNOWN** because the lifetime
`ready_since` span `< 2^31` cannot be proven from a snapshot. That is intentional.
