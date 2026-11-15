# HAEUN networking v6 (Socket API)

Status: **a minimal socket layer over the existing NIC stack (UDP sendto/recvfrom, TCP listen/accept) is implemented and tested**. Guest userland syscalls are not yet wired; this is a host-side API that still moves all frames through guest DMA.

## Milestone scope

| Component | Status |
|---|---|
| `haeun_socket` / `haeun_sock_close` | Done |
| `haeun_sock_bind` | Done |
| UDP `sendto` / `recvfrom` | Done |
| TCP `listen` / `accept` | Done |
| Full BSD connect/select, guest syscalls | Not started |

## Model

1. `haeun_net_socks` owns up to four socket slots bound to one `haeun_net_if`.
2. Datagram sockets call through `haeun_net_udp_*` and therefore the NIC TX/RX path.
3. Stream sockets use the passive TCP handshake; `accept` succeeds only after ESTABLISHED.
4. Invalid fds and type mismatches are rejected and logged.

## Tests

`tests/net_sock_tests.c` covers UDP sendto capture, inject→poll→recvfrom, and TCP listen/accept after handshake.

## Next milestone

See [net-sock-syscall-v1.md](net-sock-syscall-v1.md) for guest `ECALL` socket wiring.
