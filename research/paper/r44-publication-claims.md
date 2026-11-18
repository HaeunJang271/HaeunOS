# R4.4 Publication-Safe Claims

**Does not overwrite** `research/paper/main.md`.  
**Baseline implementation:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Docs tip:** `735013eccec1f48f814245082238b94c04f4e288`  
**Decision:** RESEARCH_READY_WITH_LIMITATIONS  
**Third-party reproduction:** NOT TESTED  
**Individual make exits:** NOT CAPTURED (chain exit 0 on author worktree)

## A. Can claim

1. HaeunOS AROS implements a **host-assisted** pipeline that evaluates guest telemetry and produces scheduling-policy recommendations for RR and preemptive FIFO.
2. Policy mutation is **safety-gated**: prepare rejects UNKNOWN/FAIL outcomes; commit is a separate step.
3. The optimizer **does not** directly authorize guest policy changes; apply goes through the reconfiguration transaction API.
4. In recorded R4.3 runs, RR↔FIFO applies committed and host policy-selection mirrors matched **61/61** post-apply samples for both directions.
5. Invalid policy targets were blocked with policy ID unchanged in the recorded rejection case.
6. Documented guest invariants (including ready_seq constraints for FIFO) are checked by host validation before prepare may succeed.

## B. Can claim with qualification

1. **Autonomy:** automated evaluation/recommendation; **apply is explicit opt-in** (SHADOW default).
2. **Safe point:** commits are designed for **between-`haeun_vm_step`** boundaries under a **single-threaded** host interpreter — not hardware atomicity, not SMP.
3. **Rollback** restores the **previous policy ID only**, not full VM/PCB/memory state.
4. **Performance:** on six fixed catalog HXF workloads, descriptive mean guest `ready_wait` favored RR over FIFO (120 successful R4.2 runs); **not** an inferential or general superiority result.
5. **Reproducibility:** author-host detached worktree rebuild of the freeze SHA completed `make clean && make all && make test` with **chain exit 0**; third-party **NOT TESTED**.
6. **Related work:** positioned near autonomic/external-control and OS reconfiguration literature (S01–S15); novelty is the **composed** research-VM story with UNKNOWN≠PASS + transactional policy apply — not each component alone.

## C. Must not claim (remove if present)

1. Statistical significance of RR vs FIFO.
2. Fairness improvement (Jain identical in R4.2 tables).
3. Generalization beyond the catalog / to Linux production.
4. Full-system rollback or crash-consistent restore.
5. SMP-safe or hardware-atomic reconfiguration.
6. Third-party independent reproduction success.
7. Always-on unsupervised autonomous control of the kernel.
8. Broad multi-policy scheduling portfolio (only two policies).
9. Profile objectives producing diverse policy choices (30/30 RR).
10. Per-profile-cell live apply for all 30 cells (`NOT_TESTED_per_cell`).

## Suggested abstract framing (safe)

We present HaeunOS AROS, a host-assisted framework that evaluates guest scheduling telemetry, recommends between RR and preemptive FIFO, and applies policy changes only through a prepare/commit path that rejects UNKNOWN validation outcomes. We demonstrate bidirectional apply with host-mirror selection agreement in our research VM, and report descriptive catalog performance without claiming inferential superiority or third-party reproduction.

## Open experiments (optional R4.5)

- External reproduction package run  
- Workload where FIFO is recommended  
- Stochastic or noisy telemetry  
- Expanded FI / rejection corpus  
- Wall-clock vs guest-tick dual reporting  
