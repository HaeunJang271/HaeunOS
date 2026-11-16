# M21 Manual Validation Checklist (Live Viewer)

Automated `m21_persistent_desktop_tests.py` proves scripted host input and guest lifecycle. It is **not** proof of physical mouse/keyboard hardware behavior.

## Prerequisites

```text
make all
python tools/build_process_desktop.py build/process_desktop.hxf
# Ensure build/app_calc.hxf, app_edit.hxf, app_files.hxf, app_filemgr.hxf, app_svc_idle.hxf exist
```

## Live session

```text
build/haeun-vm.exe build/process_desktop.hxf --apps --persist --viewer
```

| # | Check | Pass? | Notes |
|---|---|---|---|
| 1 | Viewer window opens and stays open with no app running | ☐ | Idle desktop |
| 2 | Desktop chrome/window remains after waiting several seconds | ☐ | No accidental halt |
| 3 | Click desktop client → focus; type `1` → Calculator launches | ☐ | |
| 4 | Calculator exits; viewer still open; desktop accepts input again | ☐ | |
| 5 | Key `2` launches Editor; exits without OS shutdown | ☐ | |
| 6 | Key `3` launches File Manager; exits without OS shutdown | ☐ | |
| 7 | Repeat `1` twice more | ☐ | Slot reuse |
| 8 | Key `4` (NOSPC probe): third concurrent spawn fails; desktop recovers | ☐ | Watch stderr for `spawn NOSPC` |
| 9 | Mouse click changes focused window chrome | ☐ | Visual title emphasis |
| 10 | Esc closes desktop; viewer destroys after clean guest halt | ☐ | `r1=0` expected |

## Recording

- Date: 2026-11-15
- Host OS: Windows 10 (win32 10.0.22631)
- Result: **PASS (live Win32 viewer path)** with noted PARTIAL for physical HID
- Agent-run evidence:
  - `--apps --persist --viewer` (no `--input`): `viewer: frames=39064 … focus_id=0` then STEP_LIMIT — desktop idle kept the viewer open until budget.
  - `--apps --persist --viewer --input`: spawn calc/edit/**files**/repeat/NOSPC; Esc → `guest halted … r1=0x00000000` (frame/key counts vary).
- Failures: none on scripted live viewer path.
- PARTIAL: physical keyboard/mouse hand-operation not separately recorded in this agent session (scripted host input ≠ physical HID proof).

## Explicit non-claims

- Automated tests must not be cited as proof of physical HID devices.
- Concurrent apps beyond two AS slots remain unsupported.
