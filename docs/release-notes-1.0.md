# HAEUN-OS Release 1.0.0 notes

## Highlights

- HAEUN-32 ISA v1–v4 VM with MMU/TLB, process-v1 scheduler, HFS, GUI, sockets
- M14 apps launcher (`--apps`) with calc/edit/files
- M15 isolation: AS scrub, window/fd/socket ownership
- M16 reliability: mux merge for GUI+net, exhaustion tests
- M17 guest fault containment: user fatal trap → ZOMBIE + host reap
- Release package under `dist/haeun-os-1.0.0/` with `SHA256SUMS`

## Build

```sh
make clean && make all && make test && make release
```

## Not in 1.0

See [limitations.md](limitations.md). CMake path does not cover full M7–M17; use Make.

## Verification baseline

Recorded on the Release engineering host after `make test` and package acceptance
(see final M18 report). External DNS remains SKIPPED unless explicitly enabled.
