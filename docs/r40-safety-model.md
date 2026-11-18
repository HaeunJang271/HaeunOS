# R4.0 Safety Model

## Invariants (R3.1 + R3.4)

| ID | Definition | Check |
|----|------------|-------|
| POL.recognized | Policy ∈ {0,1} | dynamic R3.1 |
| PCB.state_range | AS slots 0..5 | dynamic |
| CUR.* | curr/RUNNING consistency | dynamic (WARN edges) |
| SEQ.unique/wrap | ready_seq unique; wrap==0 for FIFO PASS | dynamic R3.4 |
| TXN.sole_writer | Only R3.2 commits policy | design + tests |
| UNKNOWN≠PASS | UNKNOWN rejects prepare | reconfig_txn |

## Assumptions

- Single-threaded host interpreter.
- Between-`haeun_vm_step` safe point.
- No SMP / no hardware atomicity claim.
- Rollback restores **policy ID** only.

## Separation

| Operation | May mutate guest scheduler? |
|-----------|-----------------------------|
| Optimize / robust / plan SHADOW | No |
| Feedback SHADOW | No |
| ADAPTIVE/APPLY/execute | Only via R3.1→R3.2 |
