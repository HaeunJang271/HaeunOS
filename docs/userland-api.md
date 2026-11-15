# Userland API (Release 1.0)

Guest syscalls via `ECALL` (`r1` = number). Process-v1 numbers:

| # | Name | Notes |
|---:|---|---|
| 0 | EXIT | ZOMBIE; host reap on mux |
| 1–4 | WRITE/READ/OPEN/CLOSE | Console + HFS assist |
| 5–10 | FORK/EXEC/WAIT/SLEEP/GETPID/YIELD | process-v1 |
| 11–17 | Socket | UDP/TCP host assist |
| 18–23 | Window / compose | GUI host assist |
| 24–26 | APP_COUNT/NAME/SPAWN | apps host |

Invalid user ECALL numbers: poisoned / `-1` (not process kill).  
Fatal traps (page fault, illegal, …): M17 ZOMBIE + reap — [fault-handling-v1.md](fault-handling-v1.md).

Details: [process-v1.md](process-v1.md), [process-hfs-v2.md](process-hfs-v2.md), [gfx-syscall-v1.md](gfx-syscall-v1.md), [net-sock-syscall-v1.md](net-sock-syscall-v1.md), [apps-v1.md](apps-v1.md).
