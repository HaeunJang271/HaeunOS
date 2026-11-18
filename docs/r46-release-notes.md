# HaeunOS AROS R4.6 Release Notes

**Codename:** Final Publication Freeze (documentation)  
**Status:** FINAL_FREEZE_READY (pending explicit commit/tag approval)  
**Implementation baseline (immutable):** `386c3a1517c49be8320c6ea263e726a6c006bb53`

## What this release is

A **publication and documentation** freeze that packages:

- R4.5 manuscript, tables, citation audit, figures F1–F5, reproduction kit, reviewer readiness  
- R4.5.1 consistency / traceability / inventory audits  
- R4.5.2 F1 SHADOW/APPLY diagram resolution  
- R4.6 release audit + manifest + these notes  

It does **not** change the guest kernel, host implementation, tests, workloads, or experimental JSON.

## Research positioning

Host-assisted AROS with explicit separation of evaluation, recommendation, safety validation, and transactional application (SHADOW default; APPLY opt-in; UNKNOWN≠PASS; prepare≠commit).

## Evidence highlights (pre-existing)

- R4.3 recorded RR↔FIFO apply with host-mirror matches 61/61 (scoped to recorded evidence/logs)  
- R4.2 descriptive catalog 120 runs; Jain RR=FIFO (no fairness-improvement claim)  
- Author-host worktree rebuild: chain exit 0; third-party **NOT TESTED**

## Must not be read as claiming

Statistical superiority; fairness improvement; third-party reproduction success; full-system rollback; SMP; hardware atomicity; unattended automatic APPLY; broad policy portfolios.

## How to build the baseline

See `research/reproduction/r45/README.md` — checkout **`386c3a1…`**, then `make clean && make all && make test`.

## Related commits already on main

| SHA | Role |
|-----|------|
| `386c3a1…` | Implementation + research freeze |
| `735013e…` | R4.3.2 reproduction evidence docs |
| `3de73d4…` | R4.4 final research audit docs |

R4.5–R4.6 documentation files remain uncommitted until approval.
