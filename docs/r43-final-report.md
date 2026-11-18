# R4.3 Final Report

## Final Status: **PARTIAL PASS**

### Closed in R4.3
- Behavioral selection (host mirror vs `curr`) under both policies after apply
- Bidirectional RR↔FIFO active apply (COMMITTED)
- Safety rejection + policy-ID rollback refresh
- External package docs/scripts
- Kernel hash match; Makefile wires `r43-active-verification-tests`

### Still open
- Immutable Git / HEAD worktree (**NOT TESTED** — dirty tree; no auto-commit)
- Third-party reproduction (**NOT TESTED**)
- Per-cell 30 live applies (**NOT TESTED**; grouped)

---

### 1. Baseline Audit — VERIFIED
### 2. Git Baseline — dirty; immutable **NOT ESTABLISHED** (HEAD `5563a86`)
### 3. Worktree Reproduction — **NOT TESTED** (blocked)
### 4. RR Selection — **OBSERVED** (post FIFO→RR: 61/61)
### 5. FIFO Selection — **OBSERVED** (post RR→FIFO: 61/61)
### 6. Active Apply — RR→FIFO & FIFO→RR **COMMITTED** + behavioral post-events
### 7. Safety Rejection — invalid/prepare; policy unchanged **OBSERVED**
### 8. Rollback — FI AFTER_WRITE; policy-ID scope **OBSERVED**
### 9. Profile Mapping — 30 cells grouped; `live_apply_per_cell=false`
### 10. External Package — ready; **THIRD-PARTY NOT TESTED**
### 11. Evidence — C2b **OBSERVED**; C4 limited; P2 **INSUFFICIENT**
### 12. Paper/Presentation — R4.3 updated
### 13. Kernel Integrity — **VERIFIED** match
### 14. Regression — **VERIFIED**
| Command | Exit |
|---------|------|
| `make clean` | 0 |
| `make all` | 0 |
| `make test` (incl. r43) | 0 |
| `./build/r43-active-verification-tests.exe build` | 0 |
### 15. Remaining — user-approved commit for HEAD wt; 3rd party; per-cell apply
### 16. Final Status — **PARTIAL PASS**
