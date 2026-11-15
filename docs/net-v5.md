# HAEUN networking v5 (DNS over UDP)

Status: **authoritative DNS A-record replies over UDP (port 53) are implemented and tested**. See [net-v6.md](net-v6.md) for the Socket API.

## Milestone scope

| Component | Status |
|---|---|
| Zone A-record registration | Done |
| DNS query parse (QNAME/QTYPE) | Done |
| DNS response (NOERROR / NXDOMAIN) | Done |
| Serve via UDP + NIC TX | Done |
| Socket API | Done in v6 |

## Model

1. `haeun_net_dns_add_a` registers a name→IPv4 mapping in a small local zone.
2. `haeun_net_dns_enable` binds UDP (typically 53) and arms the DNS server.
3. After `haeun_net_poll` queues a UDP datagram, `haeun_net_dns_serve` parses the real query bytes and transmits a real response through `haeun_net_udp_send` (guest DMA / NIC).
4. Unknown names get NXDOMAIN; known A queries get an answer section. No canned stub replies without parsing.

## Tests

`tests/net_dns_tests.c` injects a DNS query for a registered name and checks the TX sink for the matching A RDATA; also checks NXDOMAIN.

## Next networking milestone

Completed in v6 (Socket API). Networking stack host scaffolding is feature-complete for the original checklist; next is Security or guest syscall wiring.
