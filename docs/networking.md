# Networking (Release 1.0)

## Modes

| Flag | Behavior |
|---|---|
| `--net` | Loopback NIC + socket ECALL mux |
| `--net-real` | Host UDP backend (Winsock on Windows) |

Combined with `--apps`/`--gui`: sock context is merged into the GUI mux after net attach.

## Guest sockets

ECALL 11–17 (SOCKET/BIND/SENDTO/RECVFROM/…); max 4 sockets; `owner_pid` cleanup on EXIT/fault.

## External DNS

`HAEUN_NET_EXTERNAL=1` enables live external checks in `m13_net_backend_tests`.  
Default `make test` **skips** external DNS (not counted as PASS).

Details: [net-v1.md](net-v1.md)–[net-v6.md](net-v6.md), [net-real-v1.md](net-real-v1.md).
