# Guest socket syscalls v1

Status: **user `ECALL` numbers 11–17 for SOCKET/BIND/SENDTO/RECVFROM/LISTEN/ACCEPT/SO_CLOSE are completed by a host assist over the existing Socket API**, with frames still moving through guest NIC DMA. Process-v1 guests reach the same assist through `haeun_ecall_mux_io` (`--net`). GUI syscalls and secure boot remain separate.

## Syscall ABI

| # | Name | Args | Result (`r1`) |
|---:|---|---|---|
| 11 | `SOCKET` | `r2` type (`DGRAM`/`STREAM`) | fd or `-1` |
| 12 | `BIND` | `r2` fd, `r3` port | `0` / `-1` |
| 13 | `SENDTO` | `r2` fd, `r3` dst_ip, `r4` dst_port, `r5` buf VA, `r6` len | `0` / `-1` |
| 14 | `RECVFROM` | `r2` fd, `r3` buf VA, `r4` cap, `r5` optional meta VA | length / `-1` |
| 15 | `LISTEN` | `r2` fd | `0` / `-1` |
| 16 | `ACCEPT` | `r2` listen fd | connected fd / `-1` |
| 17 | `SO_CLOSE` | `r2` fd | `0` / `-1` |

`SENDTO` next-hop MAC comes from `haeun_sock_ecall_ctx.next_hop_mac` (ARP-resolved MAC in a later slice). Buffer VAs are user-checked before copy.

## Process ownership (M9)

- On `SOCKET`, the assist records `owner_pid` from the process-v1 PCB (`haeun_guest_pcb_current_pid`).
- On `SYS_EXIT`, `haeun_ecall_mux_io` closes every socket owned by that pid before the trap runs.
- Scheduler context switches do not touch host socket table state; ownership is pid-keyed.

## Wiring

- Flat / unit tests: `haeun_vm_set_ecall_io(vm, haeun_sock_ecall_io, &ctx)` after `haeun_socks_init`.
- Kernel-managed guest (`--net`): `haeun_vm_net_host_attach` binds NIC DMA at `0xB000`/`0xC000` (avoids process-v1 page tables at `0x2000`/`0x3000`), disables NIC RX IRQ (process trap only ACKs timer), installs mux→socket, and loopbacks one UDP reply after the first TX.

## Guest demo

```text
python tools/build_process_net.py build/process_net.hxf
./build/haeun-vm build/process_net.hxf --net
```

`userland/net_proc.s`: GETPID → SOCKET → BIND → SENDTO → RECVFROM → SO_CLOSE → EXIT.

## Tests

- `tests/net_sock_ecall_tests.c`: SOCKET→BIND→SENDTO TX capture; unknown number still traps poisoned.
- `tests/m9_process_net_tests.py` + `tests/m9_net_probe.c`: real process image TX/RX + PCB cleanup.

## Not claimed

- Real NIC or internet access (TX sink + inject loopback only)
- Automatic ARP for next-hop MAC
- Guest-driven NIC RX IRQ handling in process-v1 trap
- Combined `--gui --net` (last attach wins ecall_io)
