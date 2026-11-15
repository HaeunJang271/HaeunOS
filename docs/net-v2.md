# HAEUN networking v2 (IPv4 + ICMP echo)

Status: **IPv4 header validation/checksum and ICMP echo-reply are implemented and tested**. See [net-v3.md](net-v3.md) for UDP. TCP/DNS/Socket API remain deferred.

## Milestone scope

| Component | Status |
|---|---|
| IPv4 header parse + checksum | Done |
| ICMP echo request → reply | Done |
| UDP | Done in v3 |
| TCP / DNS / sockets | Not started |

## Rules

1. Echo replies are built from the received IPv4/ICMP bytes (swap addresses, type 0, recompute checksums) — not canned payloads.
2. Frames still enter and leave only through the NIC guest-DMA path (`haeun_net_poll`).
3. Packets not addressed to the local IPv4, with bad checksums, or non-echo ICMP are dropped and logged when validation fails.

## Tests

`tests/net_icmp_tests.c` injects a real ICMP echo request through the NIC and checks the TX sink for a correctly addressed echo reply.

## Next networking milestone

Completed in v3 (UDP). Next: TCP or DNS/Socket API.
