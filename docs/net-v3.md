# HAEUN networking v3 (UDP over IPv4)

Status: **UDP bind/send/recv over IPv4 through the NIC guest-DMA path are implemented and tested**. See [net-v4.md](net-v4.md) for TCP handshake. DNS/Socket API remain deferred.

## Milestone scope

| Component | Status |
|---|---|
| UDP bind (local port) | Done |
| UDP send (Eth+IPv4+UDP via NIC TX) | Done |
| UDP recv (poll delivers one datagram) | Done |
| UDP checksum (pseudo-header) | Done |
| TCP handshake | Done in v4 |
| DNS / sockets | Not started |

## Model

1. `haeun_net_udp_bind` selects the local port that `haeun_net_poll` accepts.
2. Inbound UDP to that port is copied from the guest RX DMA buffer into one pending slot (payload + peer address/port).
3. `haeun_net_udp_send` builds a real Ethernet/IPv4/UDP frame in the guest TX buffer and kicks the NIC.
4. Destination MAC is supplied by the caller (no ARP cache in M3).

## Tests

`tests/net_udp_tests.c` covers send capture, inject→poll→recv payload, and port mismatch drop.

## Next networking milestone

Completed in v4 (TCP handshake). Next: DNS over UDP or Socket API.
