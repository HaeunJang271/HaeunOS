# HAEUN guest fault containment (M17)

Status: **user fatal traps are classified, the faulting PCB becomes ZOMBIE,
host resources are reaped, and the scheduler resumes another READY process**.
Kernel-mode fatal traps log and halt. Invalid ECALL numbers still return `-1`.

## Trap architecture (audit)

| Event | Detected by | Cause | Handler |
|---|---|---|---|
| Page fault | Host MMU walk (`mmu.c`) | 5 | Guest `proc_trap.s` → `sys_fault_exit` |
| Illegal insn | Decoder/executor | 0 | same user-fatal path |
| Memory OOB | `memory_interface.c` | 1 | same |
| Alignment | `memory_interface.c` | 2 | same |
| ECALL | executor | 3 | syscall dispatch |
| Privilege | CSR/ERET/SFENCE | 4 | user-fatal (if from user) |
| Timer IRQ | IRQC source 0 | 16 | `do_timer` → `pick_next` |
| Kernel fatal | same causes, `status.PP=1` | * | `kernel_fault` → `halt` |

Host `haeun_trap_enter` records `epc`/`cause`/`tval`, sets `status.PP`, and for
**user** fatal causes invokes optional `fault_io` (mux cleanup) before the guest
handler runs.

## Classification

`haeun_trap_cause_is_user_fatal`: causes `{0,1,2,4,5}` only.
ECALL (`3`) and IRQ (`>=16`) are excluded.

## User process termination

1. Host `haeun_ecall_mux_on_fault`: log pid/cause/epc/tval; reap windows/sockets/FDs/AS.
2. Guest `sys_fault_exit`: if not already ZOMBIE, store cause in `exit_code`,
   write diag words at `0xA450` (cause, tval, pid, epc), set state ZOMBIE,
   `wake_wait`, `pick_next`, `ERET` to survivor.

## Diagnostics

| Address | Field |
|---|---|
| `0xA450` | last fault cause |
| `0xA454` | tval |
| `0xA458` | pid |
| `0xA45C` | faulting epc |

Host stderr: `[haeun-fault] user pid=…` or `[haeun-fault] KERNEL …`.

## Tests

```text
./build/m17-fault-tests.exe
python tests/m17_process_fault_tests.py build/haeun-vm.exe
```

## Collateral fix

`fd_check` previously clobbered `r3`, so `SYS_WRITE` always transmitted `0x01`.
It now uses `r5` as scratch so the WRITE data byte is preserved.

## Limits

- No user signal/restart; fatal ⇒ ZOMBIE.
- Kernel fault is fatal (halt), not recovered.
- Plain HXF runs without `--gui/--net` still contain via guest path; host reap
  requires mux/`fault_io` (installed by GUI/net/apps attach).
- Dedicated guest binaries for illegal-insn / CSR privilege faults share the same
  classifier but are not separate end-to-end images (host cause injection covers them).
