# M20 Completion Report

## Acceptance checklist

| # | Criterion | Status | Evidence |
|---|-----------|--------|----------|
| 1 | ≥2 apps concurrent | IMPLEMENTED | `process_sdk --app text --app files` dual run (no fault, halt) |
| 2 | Per-app input only | IMPLEMENTED | `haeun_evt_*` + unit isolation; chrome routes by `owner_pid` |
| 3 | Focus switch | IMPLEMENTED | `haeun_evt_on_focus` from chrome |
| 4 | IPC A↔B | IMPLEMENTED | Unit + `app_m20_ipc` process test (self send/recv + svc) |
| 5 | GUI text | IMPLEMENTED | `WIN_DRAW_TEXT` + `app_m20_text` (no fault, halt) |
| 6 | Window events | IMPLEMENTED | FOCUS/BLUR/KEY/MOUSE/CLOSE/RESIZE via poll |
| 7 | Application launcher | PARTIAL | Reuses APP_COUNT/SPAWN; no new GUI launcher shell beyond existing |
| 8 | File manager HFS | PARTIAL | `app_m20_files` create/write + text; no readdir ECALL |
| 9 | Reap on exit | IMPLEMENTED | mux `reap_pid` extends to evt/ipc/svc |
| 10 | Exhaustion protection | IMPLEMENTED | Queue drop/full errors; unit overflow test |
| 11 | M7–M19 regression | IMPLEMENTED* | M14/M15/M17/M19 + chrome re-run PASS this session (*full `make test` not wall-clocked end-to-end here) |
| 12 | Viewer integration | PARTIAL | Chrome hooks present; automated viewer dual-app scenario UNVERIFIED |

## P0 fix

`tools/build_app_hxf.py` now applies PCREL18 relocations. Calculator/text/IPC apps halt without `[haeun-fault]`.

## New ECALL 27–34

EVENT_POLL, WIN_DRAW_TEXT, IPC_*, SVC_* — `HAEUN_SYS_MAX=34`.

## Docs

- `docs/m20-architecture-audit.md`
- `docs/input-events-v2.md`, `ipc-v1.md`, `gui-text-v1.md`, `window-events-v1.md`, `services-v1.md`, `desktop-v2.md`

## Remaining limitations

- Hangul unsupported (ASCII 8×8 only). Default FB later raised to 320×180 (M22.1.3+); see [graphics-api.md](graphics-api.md).
- No blocking EVENT_WAIT syscall
- No HFS readdir
- No separate M20 release tarball (dev tree / do not overwrite 1.0.0 claims blindly)
- Full interactive 13-step desktop scenario needs manual viewer verification
