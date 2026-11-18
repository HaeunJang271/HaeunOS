# R4.3.2 Reproduction Evidence

**Milestone status: COMPLETE**  
**Audit mode:** READ-ONLY SOURCE AUDIT + DOCUMENTATION ONLY  
**Third-party reproduction: NOT TESTED**

## Baseline

| Field | Value |
|-------|-------|
| Immutable freeze SHA | `386c3a1517c49be8320c6ea263e726a6c006bb53` |
| Commit subject | `research: freeze HaeunOS R4.3 validation baseline` |
| Parent | `5563a8614820bb73f76e4ce683e339dc78f0e4c6` |
| Commit path count | 207 (matches R4.3.1 manifest INCLUDE set exactly) |

## Worktree

| Field | Value |
|-------|-------|
| Path | `../HaeunOS-r431-wt` (absolute observed: sibling of primary repo under Projects2) |
| HEAD | `386c3a1517c49be8320c6ea263e726a6c006bb53` (matches freeze) |
| Branch state | detached HEAD |
| Porcelain clean? | **NO** — artifact dirty after `make test` (see limitations) |
| Kernel/host/test sources vs freeze | Unchanged (kernel SHA-256 match; see integrity doc) |

### Worktree dirty paths (observed)

| Path | Kind | Notes |
|------|------|-------|
| `research/experiments/r42-performance-results.json` | modified tracked | 1-line numstat churn from test run |
| `r431-worktree-make.log` | untracked | full console capture of chained make |
| `env-r431-worktree.json` | untracked | host env snapshot taken before make |

## Command executed

```text
git worktree add --detach ../HaeunOS-r431-wt 386c3a1517c49be8320c6ea263e726a6c006bb53
cd ../HaeunOS-r431-wt
make clean && make all && make test
```

| Item | Evidence |
|------|----------|
| Working directory | worktree path above |
| Exit status | `CHAIN_EXIT_0; INDIVIDUAL_EXIT_CODES_NOT_CAPTURED` (~532 s) |
| Log file | `r431-worktree-make.log` (~138 KB, 1698 lines) |
| Separate per-target exit codes | **NOT CAPTURED** — only chain exit + log text available |

### Stage evidence from log

| Stage | Verdict | Evidence |
|-------|---------|----------|
| `make clean` | LOG_EVIDENCE_ONLY | Log begins with PowerShell removal/recreation of `build/`; individual exit not captured |
| `make all` | LOG_EVIDENCE_ONLY | Immediate `cc -Iinclude -std=c17 -O2 ...` builds; individual exit not captured |
| `make test` | LOG_EVIDENCE_ONLY | Suites through r43 appear in log; individual exit not captured |
| Combined chain | `CHAIN_EXIT_0` | Shell `&&` chain reported exit 0 |

## Validation suite evidence (from log)

| Suite / claim | Result | Log anchors |
|---------------|--------|-------------|
| R4.1 smoke | PASS | `r41 smoke passed` |
| R4.1 standard | PASS | `r41 standard passed` |
| R4.1 validation | PASS | `r41 validation tests passed` |
| R4.2 multi-profile | PASS | `planned=120 ok=120 fail=0 profiles=5` then `r42 validation tests passed` |
| R4.3 RR→FIFO behavioral | PASS | `RR_to_FIFO_behavioral ok matches=61 events=61 post=61` |
| R4.3 FIFO→RR behavioral | PASS | `FIFO_to_RR_behavioral ok matches=61 events=61 post=61` |
| R4.3 selection mirrors | PASS | `selection RR matches=61 FIFO-boot matches=61` |
| R4.3 active verification | PASS | `r43 active verification tests passed` |

## Environment

| Item | Value |
|------|-------|
| OS | Microsoft Windows NT 10.0.22631.0 |
| Compiler | cc.exe (x86_64-posix-seh-rev0, MinGW-Builds) 15.2.0 |
| Make | GNU Make 4.4.1 |
| Python | Python 3.13.2 |
| Timestamp (env snapshot) | 2026-11-18T18:45:45+09:00 |

### Compiler / link flags observed in log

- `CFLAGS`/`CPPFLAGS` from Makefile defaults: `-std=c17 -O2 -Wall -Wextra -Wpedantic -Werror`, `-Iinclude`
- Windows VM link libs observed: `-luser32 -lgdi32 -lws2_32`
- Net backend object: `src/net_backend_win32.c`

### Environment dependencies

- Host `cc` on PATH (MinGW)
- `make`, `python` for image stamps / builders
- No extra env vars required for the chained make (none recorded as mandatory)

## Known limitations

1. Same-machine detached worktree only — **not** independent third-party lab.
2. Per-target exit codes were not separately instrumented; only the `&&` chain exit and log text.
3. Worktree is not porcelain-clean after tests (experiment JSON + local logs).
4. Absolute host paths appear in local env/log artifacts; do not treat those artifacts as freeze inputs.
5. `r431-baseline.json` inside the freeze still reflects pre-freeze draft fields; post-freeze truth is this R4.3.2 evidence pack + commit SHA.

## Claims boundary

- **Claimed:** freeze SHA rebuilds and passes `make clean && make all && make test` on this host worktree.
- **Not claimed:** third-party reproduction, clean worktree after test, or bit-identical experiment JSON after re-run.
