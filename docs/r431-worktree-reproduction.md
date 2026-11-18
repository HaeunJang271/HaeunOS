# R4.3.1 Worktree Reproduction

## Separate status fields

| Field | Value |
|-------|-------|
| **Commit status** | `COMMIT_OK` |
| **Freeze commit SHA** | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| **Commit message** | `research: freeze HaeunOS R4.3 validation baseline` |
| **Worktree reproduction** | `WORKTREE_VERIFIED` |
| **Third-party reproduction** | `NOT_TESTED` (do not claim) |

Commit success and worktree success are recorded separately above.

## Procedure (executed)

```text
git worktree add --detach ../HaeunOS-r431-wt 386c3a1517c49be8320c6ea263e726a6c006bb53
cd ../HaeunOS-r431-wt
make clean && make all && make test
```

- Worktree path: `../HaeunOS-r431-wt`
- Detached HEAD at freeze SHA
- Primary `build/` was not copied; artifacts built inside the worktree only
- Main working tree was not used to “fix” failures (none occurred)

## Environment (worktree host)

| Item | Value |
|------|-------|
| Timestamp (local) | 2026-11-18T18:45:45+09:00 |
| OS | Microsoft Windows NT 10.0.22631.0 |
| Compiler | cc.exe (x86_64-posix-seh-rev0, MinGW-Builds) 15.2.0 |
| Make | GNU Make 4.4.1 |
| Python | Python 3.13.2 |
| `git status` in worktree | `## HEAD (no branch)` |
| Elapsed | ~532 s |
| Exit status | `CHAIN_EXIT_0; INDIVIDUAL_EXIT_CODES_NOT_CAPTURED` |

## Observed outcomes

- Chained command `make clean && make all && make test`: **chain exit 0** (individual target exits not separately captured)
- Tail evidence includes:
  - `r41 validation tests passed`
  - `r42 validation tests passed`
  - `RR_to_FIFO_behavioral ok matches=61 events=61 post=61`
  - `FIFO_to_RR_behavioral ok matches=61 events=61 post=61`
  - `r43 active verification tests passed`
- Full console capture retained in worktree as `r431-worktree-make.log` (local artifact; not part of freeze commit)

## Staging / commit notes (pre-worktree)

- Staged exactly **207** INCLUDE paths; EXCLUDE **12** remained untracked after commit
- `git diff --cached --check` exited non-zero due to CRLF (`\r`) false-positive “trailing whitespace” (0 actual trailing spaces on sampled file); freeze content unchanged
- Kernel staged diffs matched manifest: `proc_policy.s`, `proc_telem.s`, `proc_fork.s` only among process kernels

## Claims boundary

- This verifies **same-machine detached worktree rebuild/test** of the freeze commit.
- It does **not** establish independent third-party reproduction.
