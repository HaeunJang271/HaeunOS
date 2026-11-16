# M20 GUI Runtime Investigation

Date: 2026-11-15  
Mode: **read-only** (no source changes).  
Command:

```powershell
.\build\haeun-vm.exe build\process_launch.hxf --apps --viewer
```

Log under analysis:

```text
apps: registered calc … filemgr
app-env: spawn calc pid=2 slot=1
app-env: spawn edit pid=3 slot=2
app-env: spawn svcidle pid=4 slot=1
viewer: frames=398 keys=1 mice=0 focus_hits=0 focus_id=-1
guest halted after 101782 instructions; r1=0x00000001
```

No `[haeun-fault]` in this log.

---

## 1. Guest halt — path and condition

### Path (code)

1. Guest image userland is `gui_launch.s` (`tools/build_process_launch.py` → `process_launch.hxf`).
2. End of smoke script (`userland/gui_launch.s` ~115–118):

   - `SYS_EXIT` (`r1=0`, `r2=1`)
   - then architectural `halt`

3. Host EXIT assist (`src/ecall_mux.c` → `haeun_ecall_mux_io`): on `HAEUN_SYS_EXIT`, calls `reap_pid(...)`, returns `false` so guest `sys_exit` still runs.
4. Guest `sys_exit` (`kernel/process/proc_sys.s`): mark ZOMBIE(5); if `pick_next` finds no READY (after optional `wake_all`), `lw r1, 16(r8)` (exit code) then **`halt`**.
5. Host run loop (`src/main.c`): `--viewer` → `haeun_vm_viewer_run` → `haeun_vm_step` returns `HAEUN_VM_HALTED` → prints  
   `guest halted after …; r1=0x…` with **`vm.r[1]`**.

### Condition for this log

- `r1=0x00000001` matches launcher EXIT argument `r2=1` in `gui_launch.s` (success smoke code), not a random fault code.
- Instruction count ~101782 is below a typical step-limit abort path; halt is guest-driven, not `HAEUN_VM_STEP_LIMIT` (that would not print the same “guest halted” line for limit — `main.c` only prints that string for `result == HAEUN_VM_HALTED`).

**Class:** normal smoke termination (구현된 정상 종료).  
**Not:** unexplained crash. Service was STARTed then STOPped in the same script before EXIT — not a long-running daemon session.

---

## 2. PID 2 exit and PID 4 → slot 1 reuse

### Evidence that PID 2 exited (EXIT, not FAULT)

- `app_calc.s` ends with `SYS_EXIT` after `WIN_DESTROY` (no fault markers expected on success).
- Log has **no** `[haeun-fault]` from `haeun_ecall_mux_on_fault`.
- Host EXIT path: `haeun_ecall_mux_io` → `reap_pid` → `haeun_app_reap` (`src/app_env.c`): scrub code/stack/PTE, `haeun_tlb_flush`, clear `runs[slot]`.

### Why edit used slot 2 while calc was slot 1

`haeun_app_spawn` (`src/app_env.c`) picks first `slot∈[1,2]` with `!runs[slot].used` **and** PCB `STATE_FREE(0)`.

Observed: `edit pid=3 slot=2` ⇒ at edit-spawn time slot 1 was **not** selectable (either `runs[1].used` or PCB1 ≠ FREE).

Consistent mechanism (code, not live instrumentation):

- On EXIT, host clears `runs[]` early; guest PCB becomes ZOMBIE(5) in `sys_exit` until parent WAIT reaps (`kernel/process/proc_fork.s` `sys_wait` success path sets state 0).
- If parent resumes after a **blocked** WAIT via `wake_wait` (`proc_sys.s`) without re-entering `wt_scan`, zombie PCB can linger while parent continues — spawn then skips slot 1 → edit→slot2.

**Live dual use of slot 1 by PID 2 and PID 4:** spawn refuses non-FREE PCB → **not supported as concurrent live mapping**. PID 4 on slot 1 implies slot 1 met FREE+unused at `SERVICE_START`→`haeun_app_spawn` for `svcidle`.

**PID 2 fully gone before PID 4:** log order alone does not print reap; **UNVERIFIED** exact wall-clock moment without reap logs. Inferable: slot 1 free for pid 4 ⇒ prior occupant’s PCB FREE and `runs[1]` clear — calc (pid 2) no longer live in that slot.

| Aspect | Class |
|---|---|
| Slot reuse after free | 정상 동작 (IMPLEMENTED) |
| edit on slot2 while calc was slot1 | 구현 제한 / WAIT 동작 PARTIAL 가능 |
| CRITICAL live alias PID2+PID4 | 근거 없음 (spawn 가드) |

---

## 3. Why `filemgr` registered but not spawned

Host (`src/main.c` `register_builtin_apps`):

| Index (typical) | Name | Action |
|---|---|---|
| 0 | calc | registered |
| 1 | edit | registered |
| 2 | files | registered |
| 3 | svcidle | registered + `haeun_sm_register` |
| 4 | filemgr | registered only |

Guest (`userland/gui_launch.s`):

- `APP_SPAWN` index **0** (calc), then **1** (edit).
- Service path: `SERVICE_COUNT` / `SERVICE_START` id **0** → spawns **svcidle** via SM (`src/svc_mgr.c` `haeun_sm_start` → `haeun_app_spawn`), not filemgr.
- **No** `APP_SPAWN` of index 2 (`files`) or 4 (`filemgr`).

**Class:** 구현 제한 (smoke launcher does not launch filemgr). Not a registry failure — registration log proves host load OK.

---

## 4. `focus_id=-1` and `focus_hits=0`

### When printed

`src/main.c` prints viewer stats **after** `haeun_vm_viewer_run` returns (post-halt). So `focus_id` is **end-of-run** WM state, not a mid-run sample.

### How focus becomes -1

- `haeun_gfx_wm_destroy` (`src/gfx_wm.c`): if destroyed id == `focus_id`, sets `focus_id = -1`.
- App EXIT → `haeun_app_reap` → `haeun_app_destroy_windows` destroys owned windows.
- Launcher EXIT likewise destroys remaining launcher windows.

Calc/edit also destroy their own windows before EXIT (`app_calc.s` etc.). End state `-1` is **expected** after full teardown.

### `focus_hits=0`

Incremented in `src/vm_viewer_win32.c` `post_mouse` only when chrome/hit_focus **changes** focus to a valid id (≥0).  
`mice=0` ⇒ no successful `post_mouse` ⇒ `focus_hits` stays 0. Consistent.

Mid-run focus from `WIN_CREATE` (`src/gfx_ecall.c` raise+`set_focus` for `pid!=0`) is **UNVERIFIED** in this log (no per-create prints).

---

## 5. `mice=0` and mouse path

Path: `wnd_proc` (`WM_MOUSEMOVE` / button msgs) → `haeun_fb_view_map_mouse` → `post_mouse` → `haeun_vm_mouse_post` + optional `haeun_gfx_chrome_pointer` (`src/vm_viewer_win32.c`).

`mice_posted++` only inside `post_mouse` after `haeun_vm_mouse_post` succeeds.

**`mice=0` meaning:** zero mouse events counted this session — no mapped mouse posts (user did not move/click in the FB mapping, or messages never reached that path).

**Class:** 입력 없음으로 설명 가능. **Not** proof of a broken mouse pipeline; pipeline code exists. Live delivery to guest evt queues this run: **UNVERIFIED**.

---

## 6. Viewer rendering vs frame counter

`haeun_vm_viewer_run` (`src/vm_viewer_win32.c`):

1. Step VM in slices of `VIEW_SLICE` (256).
2. `pump` Win32 messages.
3. `refresh_fb`: optional `haeun_gfx_chrome_present`, `haeun_fb_view_snapshot_bgra`, then `paint` → `StretchDIBits`.
4. `frames_drawn++` inside `paint` after a blit attempt.

`frames=398` ≈ `101782 / 256` → consistent with one refresh per slice until halt, plus final `refresh_fb` on `done`.

**Class:** viewer **does** snapshot FB and blit to the Win32 window each refresh; counter is tied to `paint`, not an empty increment. Pixel-correct content of those frames vs expected UI: **UNVERIFIED** without PPM/screenshot capture this run.

`--apps` enables desktop → `viewer.chrome = &gui_host.chrome` (`main.c`), so chrome present is invoked when attached.

---

## 7. `keys=1`

Path: `WM_KEYDOWN` / `WM_KEYUP` → `MapVirtualKey` scancode → `feed_key` → `keys_fed++` + `haeun_vm_keyboard_feed` + `haeun_gfx_chrome_key` (`src/vm_viewer_win32.c`).

- Counter is **number of `feed_key` calls** with `sc != 0`, not “unique keys”.
- `keys=1` ⇒ exactly one such feed (e.g. only DOWN before halt, or only one message with nonzero sc).
- **Which VK/scancode:** **UNVERIFIED** (not logged).
- Delivery into a process evt queue requires `focus_id >= 0` in `haeun_gfx_chrome_key` / `haeun_evt_route_key` (`src/gfx_chrome.c`, `src/evt.c`). End-state focus was -1; whether focus was valid at the key instant: **UNVERIFIED**.

---

## Separation

| Kind | Items |
|---|---|
| 정상 동작 | Launcher EXIT(1)→halt; host register apps; spawn calc/edit/svcidle; viewer FB blit loop; end `focus_id=-1` after destroys; `mice=0` if no mouse posts |
| 구현 제한 | `gui_launch.s` smoke (no filemgr/files spawn); AS slots 1–2 for apps; stats printed post-halt only |
| 결함 가능 | WAIT block-wake without zombie rescan (explains edit→slot2); tracked PARTIAL — not proven CRITICAL AS alias |
| UNVERIFIED | Exact key identity; key→pid evt; mid-run focus; visual correctness of 398 frames; exact PID2 reap timestamp |

---

## Summary table

| 항목 | 판정 | 근거 | 다음 조치 |
|---|---|---|---|
| Guest halt | 정상 종료 | `gui_launch.s` EXIT(1)+halt; `proc_sys.s` `sys_exit`; `main.c` HALTED print; `r1=1` | 없음 (스모크 의도). 데몬 유지 원하면 런처 설계 변경 승인 후 |
| Process slot reuse | 정상 + PARTIAL 가능 | spawn 로그 slot1→2→1; `haeun_app_spawn` FREE 가드; WAIT/`wake_wait` 코드 | reap 로그 추가·WAIT 재스캔 수정은 별도 승인; CRITICAL 아님 |
| File Manager spawn | 구현 제한 | `main.c` register only; `gui_launch.s` SPAWN 0/1 + SM idle only | 수동/대화형 런처 또는 SPAWN filemgr 테스트 |
| Keyboard input | PARTIAL / UNVERIFIED | `keys=1`=`keys_fed` (`feed_key`); VK 미기록; evt 전달 미확인 | 키 로그 또는 수동 포커스 검증 |
| Mouse input | 입력 없음 (경로 IMPLEMENTED) | `mice=0`; `post_mouse` 미호출 | 수동으로 뷰어 위 이동/클릭 확인 |
| Window focus | 종료 상태 정상 | post-run `focus_id=-1` (`wm_destroy`/`reap`); `focus_hits=0`↔`mice=0` | 실행 중 focus 샘플링은 UNVERIFIED |
| GUI rendering | 렌더 루프 동작 / 내용 UNVERIFIED | `refresh_fb`+`StretchDIBits`; frames≈steps/256 | 스크린샷/PPM으로 내용 검증 |
