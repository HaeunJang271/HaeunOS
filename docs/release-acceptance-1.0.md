# Release 1.0 acceptance log

Host: Windows 10 (build env for M18). Commands run from `dist/haeun-os-1.0.0/` unless noted.

| Check | Result |
|---|---|
| `python scripts/verify_checksums.py` | PASS |
| `bin/haeun-vm.exe images/hello.hxf` | PASS (halt) |
| `--firmware images/boot.hfw --disk images/kernel_disk.raw` | PASS (`kernel ready`) |
| `process_apps.hxf --apps` (apps copied to `build/`) | PASS (halt; app faults contained) |
| Missing image path | FAIL path → exit 2 |
| Unknown option | usage → exit 2 |
| `make test` (dev tree) | PASS; external DNS SKIPPED |

Bit-identical rebuild of PE binaries: **not claimed** (MSVC/MinGW timestamps). Source-command reproducibility: `make clean && make all && make release`.
