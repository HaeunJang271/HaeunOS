# R2.1 Scheduler Policy Verification

Correctness and reproducibility checks for the R2.0 policy abstraction.
**No new policies. No runtime switching. No scheduler optimization.**

## 1. R2.0 architecture (verified)

`pick_next` (core) demotes, calls `policy_select`, validates, commits.
`policy_rr` only scans and returns a slot. See `docs/r21-policy-verification-audit.md`.

## 2. Policy contract

| | |
|---|---|
| ID | `0` at `0xA474` = RR; other → no-candidate |
| In | `r13` = curr |
| Out | `r1=1`,`r6=slot` or `r1=0` |
| RAM | Policy performs no stores |

Host mirror: `haeun_policy_select_host` in `src/policy_verify.c` (test/support only).

## 3. Determinism methodology

- Host fixture: identical `(policy_id, curr, states[])` → identical slot.
- Guest: two full `r12_yield` runs → identical `haeun_sched_trace` event sequences.
- Guest `policy_select` call vs host mirror on the same fixture → same slot.

## 4. Read-only methodology

After boot to user mode, plant PCB states, snapshot `0xA000`..`0xA47F`, set `pc=policy_select`, `r15=trampoline HALT` at `0x00107ff0`, run until halt. Compare PCB snapshot. Assert `timer_compare` / `timer_control` unchanged. `timer_count` may advance by exactly the number of host retires (board tick), not by guest policy stores. Trampoline write is test setup outside the snapshot window.

## 5. Scheduler trace (host-only)

`haeun_sched_trace_observe` records when `curr` or PCB0 `dispatch_count` changes. Fields: event_index, slots before/after, policy_id, success, state, dispatch_count, context_switches. No guest RAM writes, no extra guest instructions, no ECALL.

## 6. Baseline provenance

Git `HEAD` predates AROS R1.1. **No tagged pre-R2 AROS source tree** is available for binary diff.

Equivalence is against preserved R1.1/R1.2/R2.0 expectations (yield exit 22, dispatch≥40, ctx=0; pair ctx relation; slot3 exclusion). This is behavioral regression, not a formal source-to-source baseline.

`ready_wait_ticks` is **not** an exact invariant (R2.0 added policy/validate instructions).

## 7–10. Results summary (executed)

| Suite | Result |
|---|---|
| Host mirror / determinism / core rejects | pass |
| Trace equivalence (2× `r12_yield`, 40 events) | pass |
| Guest `policy_select` read-only + bad ID | pass |
| R1.3 text/JSON; no `0xA474` counter field | pass |
| R1.1 sched + timer (`COUNT+96`) | pass |
| R1.2 telem / R1.3 inspect / R2.0 policy | pass |
| `make clean` / `make all` / `make test` | exit 0 |

## 11. Limitations

- Out-of-range `r13` is not rejected; scan wraps (matches guest).
- Trace observes post-commit `curr`/dispatch, not a direct `policy_select` probe each IRQ.
- Not formal verification.

## 12. Deferred to R3+

Runtime switching, new policies, function pointers, reconfiguration, guest-visible traces.
