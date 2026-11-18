# R4.3 Worktree Audit

## Status: **NOT TESTED**

| Prerequisite | Result |
|--------------|--------|
| Immutable clean commit containing R4.x sources | **Missing** (dirty tree; HEAD=`5563a86` predates most AROS files) |
| `git worktree add` from that commit | Blocked — would omit untracked `opt_*`/`robust_*`/R4 tests |

## Alternative (not HEAD worktree)

R4.2 separate-directory working-tree copy remains prior **OBSERVED** (same host). It is **not** HEAD worktree reproduction.

## Unblock path

1. User approves commit of research sources (exclude `build/`, `*.log`).
2. Re-run `git worktree add --detach <clean-sha>`.
3. `make clean && make all && make test` in worktree.
4. Compare hashes/artifacts → update `r43-worktree-results.json`.
