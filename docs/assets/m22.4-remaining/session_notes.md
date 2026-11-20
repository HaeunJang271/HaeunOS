# M22.4 remaining items session notes
Date: 2026-11-21
Host: Windows Win32
Command: .\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
PID: 25120
StartTime: 2026-11-21 01:24:45 +09:00
WorkingDirectory: <repository-root>
No --input / --input-script used.

## Step 6 — Repeat launches
Actions (computer-use injected HID into focused HAEUN-OS Viewer after click focus):
1. Focused viewer window; pressed key 1 → Calc window opened showing 0.
2. Pressed Escape → returned to desktop tiles (Welcome / Calculator / Editor / Files).
3. Pressed key 1 again → Calc opened again showing 0 (second spawn).
4. Pressed Escape → desktop restored.
5. Pressed key 2 → Editor window opened ("Editor type Esc").
Observed: no [haeun-fault] / STEP_LIMIT seen in console during these actions.
Transient console lines observed earlier in session:
  [haeun-evt] key pid=1 sc=0x02 down/up
  [haeun-evt] key pid=2 sc=0x02 / sc=0x01 transitions
Status: PASS

## Step 7 — Mouse focus
Attempted after Editor was open. Subsequent mouse clicks often hit the covering IDE (Cursor) due to Z-order; AppActivate(25120)=False.
Protocol (click app chrome/client, then shell header, then spawn key to shell) was NOT completed with reliable visual confirmation.
Status: NOT TESTED (automation blocked by window Z-order / focus-stealing prevention)

## Step 9 — Long run idle
Process remained Responding=True with MainWindowTitle=H for >= 9 minutes from StartTime (checked ~01:33:54).
Post-soak interactive UI responsiveness could NOT be confirmed (focus could not be returned to haeun-vm from automation).
Status: NOT TESTED for full criterion; no-crash portion observed only.
