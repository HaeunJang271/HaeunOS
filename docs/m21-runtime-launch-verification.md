# M21.1 Correct Runtime Launch Verification

Date: 2026-11-15  
Scope: read-only confirmation of the M21 launch path vs the M20 `process_launch` command.  
**No code was modified for this document.**

## 1. `process_desktop.hxf` build path

| Check | Result (source-confirmed) |
|---|---|
| Makefile dedicated rule for `process_desktop.hxf` | **None.** Makefile only invokes `tests/m21_persistent_desktop_tests.py`, which builds the image in Python. |
| Official builder | `tools/build_process_desktop.py` → links `userland/desktop.s` via `build_process_image.build_with_user` |
| Documented write path | `python tools/build_process_desktop.py build/process_desktop.hxf` (`docs/m21-manual-validation.md`, `docs/m21-implementation-report.md`) |

PowerShell (from those docs, Windows layout):

```powershell
python tools/build_process_desktop.py build/process_desktop.hxf
```

Builtin apps still come from `src/main.c` `register_builtin_apps()` paths:

- `build/app_calc.hxf`
- `build/app_edit.hxf`
- `build/app_files.hxf`
- optional `build/app_svc_idle.hxf`, `build/app_filemgr.hxf`

(`docs/m21-manual-validation.md` prerequisites list the same set.)

## 2. CLI `--persist` handling

Confirmed in `src/main.c`:

- Usage string lists `[--persist]`.
- Parser: `--persist` sets `persist_mode = true`, `desktop_mode = true`, `gui_mode = true`.
- `--persist` alone does **not** set `apps_mode` / builtins; **`--apps` is still required** to register calc/edit/files/filemgr.
- When `--input` is also set and `persist_mode`: loads `haeun_vm_input_host_load_m21_script` (not the M20 script).

## 3. `--viewer` + `--persist` combination

Confirmed combinable: flags are independent.

- `--viewer` → `viewer_mode` → `haeun_vm_viewer_attach` / `haeun_vm_viewer_run`.
- `--persist` → desktop chrome + (with `--input`) M21 script.
- If both `--viewer` and `--input`: `viewer.script = &input_host` so the M21 timeline drives the live window.

No mutual exclusion in the parser.

## 4. Official live command (`docs/m21-manual-validation.md`)

```text
build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer
```

Interactive keys (`userland/desktop.s`): `1` calc, `2` edit, `3` **files**, `4` NOSPC, Esc shutdown. (`filemgr` remains a registered builtin for launcher/HFS tests, not the Files tile.)

## 5. Correct PowerShell commands (M21 only)

Repo root assumed. Image must already exist (or build first as in §1). Apps under `build/app_*.hxf` as above.

### Interactive live desktop (official manual path)

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
```

### Scripted end-to-end (calc → edit → files → repeat → NOSPC → Esc)

Same flags as the agent-recorded live path in `docs/m21-manual-validation.md` and the automated test shape in `tests/m21_persistent_desktop_tests.py` (`--apps --persist --input`), plus `--viewer`:

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer --input
```

### Headless automated (no Win32 window; test harness equivalent)

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --input
```

## 6. Not M21 — do not use for persistent-desktop verification

| Command | Milestone | Why |
|---|---|---|
| `.\build\haeun-vm.exe .\build\process_launch.hxf --apps --viewer` | **M20** | Guest is `gui_launch.s` / `process_launch.hxf`: sequential smoke then `EXIT` + halt. No `--persist`, no persistent desktop loop. |
| Any run of `process_launch.hxf` without `process_desktop.hxf` | **M20** | Retained for M20 regression only (`docs/m21-implementation-report.md`). |

**The log produced from `process_launch.hxf --apps --viewer` is an M20 (or M20.9) launcher run, not an M21 persistent-desktop run.**

## 7. M21 verification: commands and expected results

### A. Interactive (`--viewer`, no `--input`)

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
```

| Step | Action | Expect |
|---|---|---|
| Start | Viewer opens | Stdout: `apps: registered calc/edit/files/...`; window stays open with **zero** app spawns until keys |
| Idle | Wait | No `guest halted` yet; desktop remains (M20 launch smoke would halt after the scripted sequence) |
| Calc | Key `1` | `app-env: spawn calc ...`; app exits; viewer **still open** |
| Edit | Key `2` | `app-env: spawn edit ...`; return to desktop |
| Files | Key `3` | `app-env: spawn files`; return to desktop |
| NOSPC | Key `4` | stderr: `app-env: spawn NOSPC index=...`; desktop recovers |
| Shutdown | Esc | `guest halted ...; r1=0x00000000` (desktop `EXIT(0)`); viewer closes |

### B. Scripted live (`--viewer --input`)

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer --input
```

Expect (source/docs + prior recorded live run):

- `app-env: spawn calc` (at least twice — repeat in M21 script)
- `app-env: spawn edit`
- `app-env: spawn files`
- `app-env: spawn NOSPC`
- `[haeun-evt] key pid=… sc=0x01` … `0x05` downs as applicable
- `viewer: frames=… keys=6 mice=2 …` (counts from M21 script)
- `guest halted after … instructions; r1=0x00000000`
- Process exit code **0**

Instruction count should be large enough that Esc timeline ran (automated test requires ≥ 700000 before halt).

### C. Contrast: M20 launcher (for discrimination only)

```powershell
.\build\haeun-vm.exe .\build\process_launch.hxf --apps --viewer
```

Expect M20-style finish (not persistent idle): sequential spawn markers from `gui_launch`, then halt — historically `r1=0x00000001` on the smoke path — **without** requiring Esc and **without** `--persist`.

## Summary

| Item | M21 correct | M20 (incorrect for this check) |
|---|---|---|
| HXF | `build/process_desktop.hxf` | `build/process_launch.hxf` |
| Flag | `--persist` (+ `--apps`) | `--apps` only (no `--persist`) |
| Live viewer | `--viewer` (optional `--input` for script) | `--viewer` on launch image |
| Shutdown | Esc → `r1=0` | Launcher finishes and halts on its own |

**Verdict:** The command `.\build\haeun-vm.exe build\process_launch.hxf --apps --viewer` does **not** exercise M21. Use `process_desktop.hxf` with `--apps --persist` (and `--viewer` / `--input` as above).
