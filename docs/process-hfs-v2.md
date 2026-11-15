# Process ↔ HFS bridge v2 (guest path OPEN)

Status: **user `ECALL` `SYS_OPEN` with a guest path VA is completed by a host I/O assist that calls the process-HFS bridge**. Console `READ`/`WRITE` still fall through to the guest trap/UART path. Secure boot and full guest ASM path OPEN rewrite remain separate.

## ABI extension

| Call | Args | Host assist |
|---|---|---|
| `OPEN` | `r2==1` console; else `r2`=path VA, `r3!=0` create | Yes |
| `CLOSE` | `r2` fd | Yes (when hook installed) |
| `WRITE` | `r2` fd, `r3` byte | File fds only; console → trap |
| `READ` | `r2` fd | File fds only; console → trap |

Path is a NUL-terminated root basename in guest memory (optional leading `/`). Security v2 dispatch rejects path VAs outside the user window / without `U`.

## Wiring

1. `haeun_vm_set_ecall_io(vm, haeun_proc_ecall_io, &ft)`
2. On `ECALL`: sec gate → I/O hook → trap if unhandled

## Tests

`tests/proc_ecall_io_tests.c`: OPEN path create + WRITE byte + HFS readback; YIELD still traps.

## Not claimed

- Guest `proc_sys.s` rewritten to pass path VAs without the host assist
- Nested directories / cwd
