# HAEUN networking — M13 real host backend (v1)

Status: **user-mode UDP socket backend** connects guest Socket ECALLs to the
host network without rewriting the guest stack or virtual NIC.

## Phase 1 audit (read-only)

| Area | Classification | Evidence |
|---|---|---|
| Ethernet | **IMPLEMENTED** | `src/net_eth.c` (`haeun_net_eth_write_hdr`, `haeun_net_eth_type`); `tests/net_tests.c` |
| ARP | **PARTIAL** | Reply-only `haeun_net_arp_reply` in `src/net_arp.c`; no client request / table |
| IPv4 | **PARTIAL** | Fixed IHL=20, checksum, no fragments/options; `src/net_ipv4.c`, `src/net_udp.c` |
| ICMP | **IMPLEMENTED** (echo) | `haeun_net_icmp_echo_reply`; `tests/net_icmp_tests.c` |
| UDP | **IMPLEMENTED** | bind/send/recv/input; `tests/net_udp_tests.c` |
| TCP | **PARTIAL** | Passive listen→ESTABLISHED only; no active connect; `src/net_tcp.c` |
| DNS | **PARTIAL** | Authoritative A serve over UDP; not a recursive client; `src/net_dns.c` |
| Routing / next-hop | **PARTIAL** | Fixed `sock_ctx.next_hop_mac` in `src/vm_net_host.c`; no ARP resolve |
| Virtual NIC | **IMPLEMENTED** | DMA TX/RX MMIO; `src/net_drv.c` |
| Guest DMA | **IMPLEMENTED** | TX/RX PA `0xB000`/`0xC000` in `vm_net_host` |
| Socket ECALL 11–17 | **IMPLEMENTED** | `src/net_sock_ecall.c`; sec VA checks |
| Host TX sink / RX inject | **PARTIAL** (pre-M13) | Capture + loopback inject in `vm_net_host_run`; no real NIC |
| Guest wrappers | **IMPLEMENTED** | `userland/net_proc.s` via process-v1 |
| Process socket cleanup | **IMPLEMENTED** | `owner_pid` + `haeun_socks_close_by_owner` on EXIT (`ecall_mux.c`) |
| M9 tests | **IMPLEMENTED** | `tests/m9_process_net_tests.py`, `tests/m9_net_probe.c` |

Pre-M13: TX capture ≠ internet. Next-hop MAC fixed. No automatic ARP.

## Phase 2 backend selection

| Option | Fit on Windows | Cost |
|---|---|---|
| A TAP/TUN | Needs OpenVPN TAP / Wintun, admin, bridging/NAT | High |
| **B User-mode UDP sockets** | Winsock, no driver, no admin for UDP | Low |
| C Other | Not required after audit | — |

**Selected: Option B.** Guest Ethernet/IP/UDP path stays intact; the host TX
sink forwards UDP payloads with `sendto`, and RX replies are re-encapsulated
and injected into the virtual NIC.

## Architecture

```
Guest SOCKET/SENDTO ECALL
  → net_sock / net_udp_send → NIC TX DMA → tx_sink
  → haeun_net_backend_on_tx → host UDP sendto
Host peer (echo / DNS / …)
  → backend poll_rx (non-blocking) → Ethernet rebuild → inject → net_poll
  → udp_pending → RECVFROM → guest memory
```

CLI: `--net` keeps M9 loopback; `--net-real` enables Option B.

## Protocol support (host backend)

| Protocol | Host backend | Notes |
|---|---|---|
| ARP | Not required | L2 stripped at host boundary |
| IPv4/UDP | **Yes** | Payload forward + rewrap |
| ICMP | No | Raw ICMP needs privileges on Windows |
| DNS | Via UDP | Guest/external A query works as UDP:53 |
| TCP | No (real path) | Guest listen/accept remains virtual-only |

## Security / cleanup

- Guest never sees host `SOCKET` handles.
- Existing ECALL security VA checks unchanged.
- `haeun_vm_net_host_shutdown` closes the host socket (deterministic).
- EXIT still closes guest sockets by `owner_pid`.
- Malformed / non-UDP TX frames are dropped and counted.

## Tests

- `tests/m13_net_backend_tests.c` — init, malformed drop, local UDP echo, optional external DNS (`HAEUN_NET_EXTERNAL=1`).
- `tests/m13_process_net_tests.py` — guest `net_echo.s` through `--net-real` + local echo on `127.0.0.1:19007`.
- Existing M7–M12 / M9 `--net` loopback unchanged.
