# FOSDEM 2027 — A Small Interpreted OS: HaeunOS 1.0 demo checklist

Use this for a **recorded** Windows demo. Mark results only after you observe them.

Scale: PASS / FAIL / NOT TESTED / SKIPPED.

## A. Build and unit evidence

| # | Step | Command | Result |
|---|------|---------|--------|
| A1 | Build | `make all` | |
| A2 | MMU tests | `.\build\mmu-tests.exe` | |
| A3 | TLB tests | `.\build\tlb-tests.exe` | |
| A4 | Fault containment | `python .\tests\m17_process_fault_tests.py .\build\haeun-vm.exe` | |

## B. Desktop (live Win32 viewer)

Launch:

```powershell
.\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
```

Click the **HAEUN-OS Viewer** window before typing.

| # | Step | Expected | Result |
|---|------|----------|--------|
| B1 | Desktop appears | Header / tiles / taskbar | |
| B2 | Calculator | `1` or Calc tile; digits/ops; Esc closes | |
| B3 | Editor | `2` or Edit tile; type/Backspace; Esc closes | |
| B4 | Files | `3` or Files tile; **app registry names** (not a full file browser); Esc closes | |
| B5 | Shell Esc shutdown | Shell focused; Esc; clean exit | |

Scripted `--input-script` / M22.3 automation and framebuffer PPM dumps must **not** be recorded as B2–B4 PASS for physical HID.

## C. SDK (optional)

| # | Step | Notes | Result |
|---|------|-------|--------|
| C1 | `make sdk` then documented `haeun.py` new/build/verify/run | See `sdk/docs/01-getting-started.md` | |

## D. Closing narration (must say)

- Not a microkernel; modular monolithic research guest on a C17 interpreter.
- Not bare-metal / not Amiga AROS.
- Caps: AS≤3, app≤4KiB, FB 320×180, Win32 viewer.
- Physical HID: only PASS if section B was filled after live use.
