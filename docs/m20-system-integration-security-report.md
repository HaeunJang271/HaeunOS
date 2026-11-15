# M20.8 — System Integration & Security Audit

Date: 2026-11-15  
Scope: ECALL 0–50 ABI audit, concurrent capacity, ownership security, resource
exhaust/recover, repeat loops, HFS integrity, M7–M20.7 regression.  
**No M21+ features. No ABI renumbering.**

## Verdict

| Criterion | Result |
|---|---|
| ECALL 0–50 ABI audit | **DONE** (code + unit map) |
| Integration under AS limits | **PASSED** (slots=3; full 8-way concurrent **NOT feasible**) |
| Isolation / ownership | **PASSED** (host + process tests) |
| Resource exhaust → reclaim | **PASSED** |
| Repeat loops | HFS/evt/win/ipc/sock **100×**; service start/stop **40×** |
| HFS remount integrity | **PASSED** (image-backed) |
| CRITICAL / HIGH open | **0** |
| Report | this file |

## 1. Concurrent capacity (actual)

| Resource | Cap | Concurrent note |
|---|---|---|
| AS / app runs | `HAEUN_APP_SLOTS=3` | launcher + **2** apps max |
| PCB bank | `NPROC=4` | FORK scans 0..2 only |
| App registry | `HAEUN_APP_MAX=5` | CLI `--app` / `--apps` |
| Windows | 8 | |
| IPC endpoints / queue / msg | 8 / 8 / 32 B | |
| Evt slots / queue | 8 / 16 | drop-oldest on overflow |
| FD table | 4 global | `owner_pid` on ECALL |
| Sockets | 4 | |
| Services | `HAEUN_SM_MAX=4` | |
| NIC RX host | PA `0xC000`, cap 512 | |
| FB | host-configured (tests 64×32 / 32×16) | |
| HFS | mkfs image (512 B blocks) | persists in attached image |

Requested simultaneous set (launcher+SM+filemgr+calc+edit+IPC+input+net) exceeds
AS slots. Verified: launcher multi-spawn (calc/edit/svcidle) + sequential
filemgr/IPC process tests. Documented as **PARTIAL** concurrency.

## 2. ECALL ABI map (r1=number/ret; a0=r2…a5=r7)

Error convention unless noted: `r1=0xffffffff` deny/fail; success `0` or handle.
IPC SEND full → `0xfffffffd`; bad len → `0xfffffffe`.

| # | Name | In | Out | U-mem | Auth | Owner / notes | Tests |
|---|---|---|---|---|---|---|---|
| 0 | EXIT | a0=code | halt path | no | user | mux reap wins/ipc/evt/fd/sock | m17 |
| 1 | WRITE | a0=fd a1=byte | 0/−1 | no | fd owner | FILE fd only on host assist | proc-ecall |
| 2 | READ | a0=fd | byte/−1 | no | fd owner | | proc-ecall |
| 3 | OPEN | a0=path/1 a1=creat | fd/−1 | path R | sets owner | | proc-ecall,m20.7 |
| 4 | CLOSE | a0=fd | 0/−1 | no | fd owner | | |
| 5 | FORK | — | child/parent | no | guest | AS≤3 | m8 |
| 6 | EXEC | image | — | X VA | sec validate | | sec |
| 7 | WAIT | — | status | no | guest | | |
| 8 | SLEEP | ticks | 0 | no | guest | | |
| 9 | GETPID | — | pid | no | guest | | |
| 10 | YIELD | — | 0 | no | guest | | |
| 11 | SOCKET | a0=type | fd/−1 | no | set owner | | net_sock* |
| 12 | BIND | a0=fd a1=port | 0/−1 | no | sock owner | | |
| 13 | SENDTO | fd,ip,port,buf,len | 0/−1 | buf R | sock | len≤UDP_MAX | |
| 14 | RECVFROM | fd,buf,cap,meta | n/−1 | buf W | sock | | |
| 15 | LISTEN | fd | 0/−1 | no | sock | | |
| 16 | ACCEPT | fd | nfd/−1 | no | sock; set owner | | |
| 17 | SO_CLOSE | fd | 0/−1 | no | sock | | |
| 18 | WIN_CREATE | x,y,w,h,pa | id/−1 | no | set owner; PA pool | | gfx,m20.0.2 |
| 19 | WIN_DESTROY | id | 0/−1 | no | win owner | DESTROYED evt | m20.4 |
| 20 | WIN_FILL | id,rgba | 0/−1 | no | win | | |
| 21 | WIN_PIXEL | id,x,y,c | 0/−1 | no | win | | |
| 22 | WIN_RAISE | id | 0/−1 | no | win | | |
| 23 | GFX_COMPOSE | flags | 0/−1 | no | any user | | |
| 24 | APP_COUNT | — | n | no | | | m14,m20.5 |
| 25 | APP_NAME | idx,buf,cap | 0/−1 | buf W | | | |
| 26 | APP_SPAWN | idx | pid/−1 | no | AS free | | |
| 27 | EVENT_POLL | — | packed/− | no | pid queue | | m20.1 |
| 28 | WIN_DRAW_TEXT | id,x,y,rgb,len,va | 0/−1 | text R | win | | m20.3 |
| 29 | IPC_CREATE | — | epid/−1 | no | set owner | | m20.2 |
| 30 | IPC_SEND | epid,buf,len | 0/fe/fd/ff | buf R | sender=cur pid | | |
| 31 | IPC_RECV | epid,buf,cap | n; r2=sender | buf W | ep owner | | |
| 32 | IPC_CLOSE | epid | 0/−1 | no | ep owner | | |
| 33 | SVC_REGISTER | name,epid | 0/−1 | name R | IPC name registry | **≠ SM** | m20.2 |
| 34 | SVC_LOOKUP | name | epid/−1 | name R | | | |
| 35 | IPC_AVAILABLE | epid | n/−1 | no | ep owner | | |
| 36 | IPC_PEEK | epid | meta/−1 | no | ep owner | | |
| 37 | WIN_QUERY | id | geom/−1 | no | win owner | | m20.4 |
| 38 | APP_INFO | idx,field,buf,cap | 0; flags@r2 | buf W | | | m20.5 |
| 39 | APP_RUN | slot | 0; pid/idx/state/win | no | | | |
| 40 | APP_CTRL | op,target | 0/−1 | no | parent/self | | |
| 41 | SERVICE_COUNT | — | n | no | SM table | **≠ 33–34** | m20.6 |
| 42 | SERVICE_INFO | id,field,buf,cap | 0/−1 | buf W | | | |
| 43 | SERVICE_START | id | pid/−1 | no | | | |
| 44 | SERVICE_STOP | id | 0/−1 | no | caller pid | | |
| 45 | SERVICE_STATUS | id | 0; state/pid/… | no | | | |
| 46 | SERVICE_RESTART | id | pid/−1 | no | caller | | |
| 47 | FS_READDIR | buf,cap | nbytes/−1 | buf W | | | m20.7 |
| 48 | FS_UNLINK | path | 0/−1 | path R | | | |
| 49 | FS_STAT | path | 0; size@r2 mode@r3 | path R | | | |
| 50 | FS_SEEK | fd,off | 0/−1 | no | fd owner | | |

**Collision check:** 33–34 = IPC service **name** registry (`svc.c`); 41–46 =
Service Manager (`svc_mgr_*`); 47–50 = FS assists. Numbers unique; `SYS_MAX=50`.
ABI left unchanged this milestone.

## 3. Integration & security tests

| Suite | What |
|---|---|
| `m20_integration_tests` | ABI caps, ownership, exhaust/reclaim, HFS 100×+remount, evt 100×, SM×40, sock/IPC/FD, win/ipc/sock×100 |
| `m20_integration_process_tests.py` | launcher `--apps` + sequential filemgr |
| Prior M20.* process tests | input/IPC/text/window/launcher/file |

Security cases exercised: foreign win/IPC/APP_CTRL/evt; IPC queue full + bad len;
stale win after destroy; sock wrong-owner reap; FD reap by owner; bad path/fd.
Victim data verified intact after denied ops.

**MEDIUM (documented, not CRITICAL):** host `haeun_sock_close` / `haeun_proc_read`
APIs do not enforce ownership — **ECALL** paths do. Process isolation relies on
ECALL + mux reap.

## 4. Resource exhaust protocol (executed)

For Window, IPC ep, FD, AS, Socket (via m16/m20.0.2/m20.8): fill → reject →
release → realloc → OK. Evt overflow drop-oldest; IPC send −2 + drops++.

## 5. Repeat / integrity

| Loop | Count | Result |
|---|---|---|
| HFS create/write/read/unlink | **100** | pass |
| HFS remount + reread `keep` | 1 | pass (`OK`) |
| Evt overflow push | **100** | pass |
| Win/IPC/Sock create-destroy | **100** | pass |
| Service start/stop/reap | **40** | pass (not 100; slot churn) |

VM restart persistence: **image-backed HFS remount** verified. Full host process
restart with disk file is same mechanism when `--disk` attached; not separately
re-run this session beyond remount.

## 6. Bugs

| Sev | Item | Action |
|---|---|---|
| — | none CRITICAL/HIGH found this audit | — |
| MEDIUM | host API vs ECALL ownership gap | document; ECALL remains gate |
| LOW | service repeat capped at 40 | recorded honestly |
| LOW | cannot run all apps concurrent | AS=3 limit |

## 7. Feature matrix (post M20.7+audit)

| Area | Status |
|---|---|
| ECALL 0–50 + sec gate | IMPLEMENTED |
| GUI / WM / chrome / text | IMPLEMENTED |
| Input evt queues + focus | IMPLEMENTED |
| IPC 29–36 + name svc 33–34 | IMPLEMENTED |
| App launcher 38–40 | IMPLEMENTED |
| Service Manager 41–46 | IMPLEMENTED |
| FS assists 47–50 + File Manager app | IMPLEMENTED |
| Socket ownership + reap | IMPLEMENTED |
| Full multi-app simultaneous | NOT IMPLEMENTED (AS limit) |
| Idle dedicated PCB | PARTIAL (wake_all retry) |
| HFS ACL / uid | NOT IMPLEMENTED (mode bits) |
| Korean glyphs | NOT IMPLEMENTED |

## 8. Regression

`make test` (this session) **PASSED** end-to-end, including M7–M20.7 process
suites, M20 unit/resource/input/ipc/text/window/launcher/service/file tests,
`m20_integration_tests`, and `m20_integration_process_tests.py`.  
Log: `build/m20_8_regression.log` (exit 0).

## 9. Commands

```text
make build/m20-integration-tests.exe && ./build/m20-integration-tests.exe
python tests/m20_integration_process_tests.py build/haeun-vm.exe
make test
```
