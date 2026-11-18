# R4.2 Reproduction Audit

| Kind | Result | Classification |
|------|--------|----------------|
| Same-tree `r42-validation-tests` | exit 0; 120/120 | **OBSERVED** |
| Separate-directory clean build (`run-r42-reproduction.ps1`) | exit_r42=0, exit_r41_smoke=0 | **OBSERVED** |
| Git worktree from HEAD | Not used — R4 sources uncommitted | **NOT TESTED** |
| Third-party independent lab | Not performed | **NOT TESTED / NOT CLAIMED** |

Artifacts: `research/reproduction/r42-environment.json`, `r42-manifest.json`, `r42-reproduction-run.log`.

Same host as development. Separate directory ≠ third-party independence.
