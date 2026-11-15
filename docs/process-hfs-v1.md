# Process ↔ HFS bridge v1

Status: **a host-side process file-table API that maps OPEN/READ/WRITE/CLOSE-style operations onto HFS (plus console fds) is implemented and tested**. Guest `proc_sys.s` still opens console device id `1` only; wiring path-based OPEN through the guest trap path is the next slice.

## Goal

Give the process model a real file backend so the shell’s HFS commands and process fds share one story: 4 fds per table, console or root basename files, offset-tracked read/write.

## API

| Call | Role |
|---|---|
| `haeun_proc_ft_init` | Bind optional mounted `haeun_hfs` |
| `haeun_proc_open_console` | Process-v1 console fd |
| `haeun_proc_open_path` | Root basename open/create (`/name` or `name`) |
| `haeun_proc_read` / `write` | HFS-backed transfers; advance offset |
| `haeun_proc_close` | Free slot |

Nested paths are rejected in v1. Failures log to stderr (`[haeun-proc-hfs]`).

## Tests

`tests/proc_hfs_tests.c`: console fd lifecycle, create/write/reopen/read round-trip, missing/nested/exhaustion denials.

## Next

See [process-hfs-v2.md](process-hfs-v2.md) for guest path `SYS_OPEN` via host ECALL I/O assist.

## Not claimed (v1)

- Guest ASM `SYS_OPEN` path strings (v2 host assist)
- Per-process cwd / mkdir via syscall
- Permission bits beyond what HFS already enforces on read/write
