# R3.6 Safety

- Optimizer never writes `0xA474`, PCB, READY seq, PTBR, or timer.
- OPTIMIZE is offline evaluation only.
- APPLY requires valid SELECT recommendation + fresh R3.1 + R3.2 commit.
- FAIL/UNKNOWN → reject; WARN follows R3.1 contract.
- Rollback = policy ID restore only (R3.2).
