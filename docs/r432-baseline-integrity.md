# R4.3.2 Baseline Integrity

**Milestone status: COMPLETE**  
**Freeze SHA:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Third-party reproduction: NOT TESTED**

## 1. Commit identity

| Check | Result |
|-------|--------|
| `git rev-parse HEAD` on main | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| Detached worktree HEAD | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| Commit message | `research: freeze HaeunOS R4.3 validation baseline` |
| Parent | `5563a8614820bb73f76e4ce683e339dc78f0e4c6` |
| Files changed in commit | **207** |

## 2. Manifest ↔ commit path set

Compared `docs/r431-commit-manifest.md` INCLUDE set (14 tracked + 193 untracked bullets) to `git diff-tree --name-only -r 386c3a1…`.

| Metric | Value |
|--------|-------|
| Manifest INCLUDE | 207 |
| Commit paths | 207 |
| only_manifest | [] |
| only_commit | [] |
| sets_equal | **True** |

## 3. Working tree state (main)

| Observation | Value |
|-------------|-------|
| Branch | `main` ahead of `origin/main` by 1 (freeze commit) |
| Code/kernel/host/test diffs vs HEAD | **None** |
| Documentation dirty | `docs/r431-worktree-reproduction.md` (R4.3.1 post-commit record) |
| Untracked | R4.3.1 EXCLUDE 12 logs/JSON only |
| R4.3.2 new docs (this audit) | untracked until user approves commit |

**Verdict:** no new implementation/source changes on main relative to freeze; only documentation / excluded local artifacts.

## 4. Worktree state

| Check | Result |
|-------|--------|
| Listed via `git worktree list` | yes — detached at freeze SHA |
| Porcelain clean | **NO** (artifact dirty) |
| Kernel sources vs freeze SHA-256 | **match** (section 5) |

Dirty paths are test/log artifacts, not scheduler/policy/host source edits.

## 5. Kernel SHA-256 (commit = main WT = worktree)

| File | SHA-256 | commit==main | commit==worktree |
|------|---------|--------------|------------------|
| `kernel/process/proc_sched.s` | `f378293d24ef74fe9af42615959e40c5c1de038d49a38590adfa562bf200d7d7` | YES | YES |
| `kernel/process/proc_policy.s` | `38a79c49d2f9f9d72589f2c13e63f96c1b279bb051e02ac8bb1328f7d81299de` | YES | YES |
| `kernel/process/proc_trap.s` | `e6cb2a9823a5e6a4d3bcba1322fcaad4e4732fb3138972df7cef672ebe3bc020` | YES | YES |
| `kernel/process/proc_telem.s` | `2d7dbeacdb4326838487a580f946de660a39c141c408e6767d529a89c06a705a` | YES | YES |
| `kernel/process/proc_fork.s` | `d895e560f52a33610fb9f63271d854bb38d7941422bc9a4eb093f57ed60ca603` | YES | YES |

These match the R4.3 / R4.3.1 research baseline hashes for the ready_seq kernel surface.

## 6. Integrity statements

1. Freeze commit SHA is exactly `386c3a1517c49be8320c6ea263e726a6c006bb53`.
2. Detached worktree HEAD matches that SHA.
3. Worktree is **not** porcelain-clean after reproduction (documented).
4. Kernel five-file SHA-256 matches across commit, main checkout, and worktree.
5. R4.3.1 INCLUDE manifest and commit file list are identical sets of 207 paths.
6. R4.3.2 introduced documentation only; no kernel/host/scheduler/policy/optimizer/test-source edits.

## 7. Completion checklist

| # | Condition | Met? |
|---|-----------|------|
| 1 | Baseline SHA match | YES |
| 2 | Worktree state verified | YES (HEAD match; dirty artifacts recorded) |
| 3 | Test result evidence secured | YES (`r431-worktree-make.log`) |
| 4 | Kernel hashes match | YES |
| 5 | Three R4.3.2 docs present | YES |
| 6 | No source changes in this step | YES |
