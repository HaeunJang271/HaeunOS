# HAEUN-OS 1.0.0 notes (Unreleased development version)

**Status:** [`VERSION`](../VERSION) is `1.0.0`, but this is an **Unreleased development version**. There is **no** product Git tag `v1.0.0` and **no** official GitHub product Release for `1.0.0`. The AROS research tag `v4.6.0` is separate and must not be read as product `1.0.0`.

Any local tree under `dist/haeun-os-1.0.0/` is a **historical** packaging artifact; its source-commit provenance is **not** confirmed. Do not treat it as an official published release.

## Highlights

- HAEUN-32 ISA v1–v4 VM with MMU/TLB, process-v1 scheduler, HFS, GUI, sockets
- M14 apps launcher (`--apps`) with calc/edit/files
- M15 isolation: AS scrub, window/fd/socket ownership
- M16 reliability: mux merge for GUI+net, exhaustion tests
- M17 guest fault containment: user fatal trap → ZOMBIE + host reap
- Historical local package layout under `dist/haeun-os-1.0.0/` with `SHA256SUMS` (not an official Release asset)

## Build

```sh
make clean && make all && make test && make release
```

## Not in 1.0

See [limitations.md](limitations.md). CMake path does not cover full M7–M17; use Make.

## Verification baseline

Recorded on the Release engineering host after `make test` and package acceptance
(see final M18 report). External DNS remains SKIPPED unless explicitly enabled.

That M18 / acceptance baseline is a **historical** recorded result. It is **not** the same as a current-`HEAD` official product release validation, and it does **not** establish a confirmed release commit for `v1.0.0`.
