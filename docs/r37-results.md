# R3.7 Results

## VERIFIED

- `make clean` / `make all` / `make test` exit 0
- Unit: preset/seed/hash, replay classes, reliability, schema reject
- Guest smoke pipeline + export + live R3.2 stress with FI reject
- Kernel SHA-256 unchanged vs R3.6 baseline (full digests in pre-audit)

## OBSERVED (smoke, n=3)

- Same-config replay EXACT_MATCH for all six workloads
- Weight perturbation (±10%/±50% on first objective) → STABLE rankings
- R3.6-style SELECT recommendations CONDITIONALLY/CONFIRMED under smoke rules
- Reconfig stress: invalid target rejected; alternating commits; FI before-commit hit

## INSUFFICIENT EVIDENCE

- Inferential significance / effect sizes
- standard/extended/research presets (not required for DoD; smoke executed)
- Parametric workload sweeps (NOT_SUPPORTED by fixed HXF catalog)

## NOT TESTED

- 100-rep research preset wall-clock campaign
- Cross-machine reproducibility of host binaries
