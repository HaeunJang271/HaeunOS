# R3.4 Test Report

Status: **Demonstrated** (executed results below). Date: 2026-11-17.

## Suites

| Suite | Result | Evidence |
|-------|--------|----------|
| `r34-active-unit-tests` | PASS | seq FIFO mirror; planted RR→FIFO COMMITTED; wrap→UNKNOWN reject |
| `r34-active-guest-tests` | PASS | see active transitions |
| R1.1–R3.3 regression | via `make test` | full harness |

## Active guest transitions (executed)

| Case | Image | Result | Notes |
|------|-------|--------|-------|
| RR→FIFO | `build/r12_pair.hxf` | PASS | switch step≈22371, eligible=2, post-target trace events≥1 |
| FIFO→RR | `build/fcfs/r12_pair.hxf` | PASS | switch step≈22371, eligible=2 |
| Bidirectional | `build/r12_pair.hxf` | PASS | RR→FIFO then FIFO→RR; eligible≥2 both commits |

Boot note: `0xA474` is 0 until `sched_pol_init`; tests wait for live `boot_pol` before request.

## Success criteria

1. Multiple eligible processes at commit — **yes** (eligible=2).
2. Transition while eligible exist — **yes**.
3. R3.1 validation on coherent snapshot — **yes** (overall PASS).
4. Commit at between-step safe point — **yes** (host `haeun_recfg_commit`).
5. New policy used after commit — **yes** (policy word + trace `policy_id`).
6. Trace reflects new policy — **yes** (`post_target_policy_events`).
7. Invariants valid — **yes** (UNKNOWN/FAIL reject path tested in unit wrap case).
8. Auditable — **yes** (txn log + host sched trace).

## Unit details (executed)

- `seq_fifo_mirror` — PASS
- `active_rr_to_fifo_planted` — PASS
- `wrap_rejects` — PASS (overall UNKNOWN)

## Final verification (executed)

| Command | Result |
|---------|--------|
| `make clean` | PASS |
| `make all` | PASS |
| `make test` | PASS (includes R1.1–R3.3 + R3.4 unit/guest) |

## Not claimed

- Hardware/SMP atomicity.
- Unbounded ordering without wrap sticky.
- Full-VM rollback.
