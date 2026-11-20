# M22.4 retry — Steps 7 and 9 (2026-11-21)

Command: .\build\haeun-vm.exe .\build\process_desktop.hxf --apps --persist --viewer
PID: 15220
No --input / --input-script.

## Step 7 PASS
1. Minimized Cursor (ShowWindow SW_MINIMIZE; not killed).
2. Focused viewer; opened Editor (key 2). Evidence: step7_01_editor_open.png
3. Clicked Editor client; typed `x`. Evidence: step7_02_after_key_in_app.png (cyan `x` in Editor). stderr: key to pid=2 sc=0x2d.
4. Clicked shell header; pressed `1`. Evidence: step7_03_after_shell_key1.png (Calc opened). console: app-env: spawn calc pid=3.
5. viewer log: mice=51 focus_hits=2 keys=8

## Step 9 PASS
1. Idle begin 01:40:46; end 01:45:57 (5.17 minutes). Responding=True, title=H throughout (step9_begin.txt / step9_end.txt).
2. After idle screenshot: step9_01_after_idle.png (UI still rendered; Calc still present).
3. Post-soak Esc delivered to shell; guest halted cleanly: `guest halted after 99173116 instructions; r1=0x00000000` (viewer_console.log). Process exited (window rect became invalid) — UI responded; graceful halt.

Step 6 PASS from earlier session unchanged.
