# R4.3 Git Baseline

## Investigation (executed)

| Item | Value |
|------|-------|
| Branch | `main` |
| HEAD | `5563a8614820bb73f76e4ce683e339dc78f0e4c6` (`feat: R3.3부터`) |
| Working tree | **DIRTY** |
| Staged | empty |

## Classification of dirty content

| Class | Examples |
|-------|----------|
| Source (tracked M) | `Makefile`, `main.c`, `policy_verify.c`, `pval_*`, kernel `proc_policy/telem/fork.s` |
| Source (untracked) | `src/opt_*`, `robust_*`, `planner_*`, `feedback_*`, R3.3–R4.2 tests |
| Documentation | `docs/r33*`…`r42*`, `research/` |
| Temp logs | `build_*.log`, `build_test*.log` |
| Build artifacts | `build/` (ignored) |

## Immutable baseline status

**NOT ESTABLISHED.** Dirty working tree cannot be labeled immutable.

Per R4.3 rules: no auto-commit, reset, clean, or push without user approval.

HEAD worktree reproduction from `5563a86` would **omit** R3.4–R4.2 sources → **NOT TESTED** until a commit (or approved snapshot) includes them.

## Next step for immutable baseline

User approval to commit research sources (excluding logs/`build/`), then re-run HEAD worktree.
