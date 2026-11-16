# M19 Final Report — Developer SDK

## 1. SDK architecture — IMPLEMENTED

- In-tree `sdk/` + staged `dist/haeun-sdk-1.0.0/` via `make sdk`
- Reuses `haeun_toolchain`, `haeun_isa`, `build_app_hxf`, `libapp.s`, `process_sdk` runner
- No new ISA / HXF / ECALL ABI

## 2. CLI commands — IMPLEMENTED

| Command | Behavior |
|---------|----------|
| `haeun new` | Template project |
| `haeun build` | ASM → app HXF (+ libapp) |
| `haeun verify` | Magic/ISA/entry/4KiB/RX segment |
| `haeun run` | `haeun-vm process_sdk.hxf --app …` |

## 3. Standard Library — IMPLEMENTED

`userland/libapp.s` / packaged `lib/libapp.s`: exit, getpid, yield, wait, win_*, compose, count, spawn, putc, open/read/write/close.  
**M19 baseline:** WIN_DRAW_TEXT / EVENT_POLL were not yet on the public SDK surface.  
**Current:** available via `libapp.s` — see [sdk/docs/06-graphics-api.md](../sdk/docs/06-graphics-api.md) and [graphics-api.md](graphics-api.md).

## 4. HXF build pipeline — IMPLEMENTED

`combine_sources` → assemble → single segment @ `0x00108000`, ISA 4, ≤4KiB.

## 5. Templates — IMPLEMENTED

`sdk/templates/basic` + `haeun.json`.

## 6. Example applications — IMPLEMENTED

hello, calculator, file_viewer, drawing, procinfo (separate HXFs under `sdk/examples/`).

## 7. Tests — IMPLEMENTED

`tests/m19_sdk_tests.py`: verify rejects, example build/verify, hello run, independent package test. Wired into `make test`.

## 8. Independent developer test — IMPLEMENTED

Temp copy of `dist/haeun-sdk-*` only: `new` → `build` → `verify` → `run` → halt.

## 9. Documentation — IMPLEMENTED

`sdk/docs/` installation through limitations (+ Phase 0 audit `docs/sdk-audit-m19.md`).

## 10. Remaining limitations — DOCUMENTED

No compiler; 4KiB; no text/event APIs; flat HFS paths; ≤4 `--app` (`HAEUN_APP_MAX`); ABI frozen.

### Regression

| Check | Result |
|-------|--------|
| M14 apps | PASS |
| M15 isolation | PASS |
| M17 fault | PASS |
| M18 images | PASS (`package_release --images-only`) |
| M19 SDK | PASS |
