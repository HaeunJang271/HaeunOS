# R4.3 External Reproduction Package

## Scope

Documents how an external researcher *could* rebuild and verify.  
**THIRD-PARTY REPRODUCTION: NOT TESTED** (no external lab run).

## Prerequisites

- C17 toolchain, GNU Make, Python 3
- Windows: WinSock (+ optional GDI)
- Prefer a **clean git commit** containing R3.4–R4.2 sources (currently often uncommitted)

## Commands

```bash
git clone https://github.com/HaeunJang271/HaeunOS
cd HaeunOS
# checkout the research baseline commit once published
make clean
make all
make test
./build/r43-active-verification-tests build   # or .exe on Windows
```

Or: `pwsh -File research/reproduction/run-r43.ps1`

## Outputs

- `research/experiments/r43-*.json`
- `research/reproduction/r43-artifact-manifest.json`

## Limits

Dirty tree / incomplete remote commit blocks true external HEAD reproduction until published.
