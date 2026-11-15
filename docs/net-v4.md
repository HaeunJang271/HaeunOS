# HAEUN networking v4 (TCP handshake skeleton)

Status: **passive TCP listen, SYN→SYN-ACK, and completing ACK→ESTABLISHED are implemented and tested**. Full TCP (retransmit, window, data streaming) remains deferred. See [net-v5.md](net-v5.md) for DNS.

## Milestone scope

| Component | Status |
|---|---|
| TCP listen on a local port | Done |
| SYN → SYN-ACK reply via NIC | Done |
| Final ACK → ESTABLISHED | Done |
| TCP data / retransmit / window | Not started |
| DNS | Done in v5 |
| Socket API | Not started |

## Model

1. `haeun_net_tcp_listen` puts the interface in LISTEN on a port.
2. An inbound SYN to that port builds a real SYN-ACK (checksummed) into the TX path.
3. A matching ACK advances the single connection slot to ESTABLISHED.
4. One connection only; RST/FIN and payload transfer are out of scope for M4.

## Tests

`tests/net_tcp_tests.c` injects SYN then ACK through the NIC and checks SYN-ACK bytes plus ESTABLISHED state.

## Next networking milestone

Completed in v5 (DNS). Next: Socket API.
