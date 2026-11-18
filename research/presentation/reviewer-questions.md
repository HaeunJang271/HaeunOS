# R4.3 Reviewer Questions

| Q | Evidence-based answer |
|---|------------------------|
| How was real policy switch confirmed? | `haeun_recfg_commit` → COMMITTED; `0xA474` readback; continued `haeun_vm_step` |
| Why is policy ID alone insufficient? | Must match selection rule; R4.3 compares host mirror expected slot to observed `curr` |
| How compare RR vs FIFO selection? | Post-apply traces under each target; 61+61 mirror matches |
| What if UNKNOWN? | Prepare/commit blocked; policy unchanged |
| Rollback scope? | Policy ID restoration only; not full VM |
| Independent vs clean-room copy? | HEAD worktree needs clean commit; R4.2 copy ≠ HEAD wt |
| Why R4.2 all-RR recommendations? | Catalog metrics/weights preferred RR; not universal optimum |
| Why not generalize ready_wait? | Fixed HXF; descriptive; no inferential claim |
