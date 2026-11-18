# R4.2 Reproduction Procedure

## Kinds of reproduction

| Kind | Status |
|------|--------|
| Same directory repeat | Runnable via `make test` / `r42-validation-tests` |
| Separate directory clean build | Scripted: `run-r42-reproduction.ps1` / `.sh` |
| Git worktree from HEAD | **NOT USED** — R3.x–R4.x sources largely uncommitted vs `origin/main` |
| Third-party independent lab | **NOT TESTED / NOT CLAIMED** |

## Commands (primary tree)

```sh
make clean
make all
make test
./build/r42-validation-tests.exe build
./build/r41-validation-tests.exe build both
```

## Separate-directory script

```powershell
pwsh -File research/reproduction/run-r42-reproduction.ps1
```

Produces `r42-environment.json`, `r42-manifest.json`, and copies fresh `r42-*.json` experiment artifacts.

## Integrity notes

- Do not reuse R4.1 JSON as R4.2 results.
- Guest ticks ≠ wall-clock.
- Dirty working tree must be disclosed in environment JSON.
