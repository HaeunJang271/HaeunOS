# HAEUN networking v1 (NIC + Ethernet + ARP)

Status: **virtual NIC MMIO, Ethernet frame TX/RX through guest DMA, and ARP request/reply handling are implemented and tested**. See [net-v2.md](net-v2.md) for IPv4/ICMP. UDP/TCP/DNS/Socket API are **not** implemented.

## Milestone scope

| Component | Status |
|---|---|
| Virtual NIC (`0x10007000`) | Done |
| Ethernet II TX/RX via guest RAM DMA | Done |
| ARP request parse + reply build | Done |
| IPv4 / ICMP | Done in v2 |
| UDP | Done in v3 |
| TCP handshake | Done in v4 |
| DNS | Done in v5 |
| Socket API | Done in v6 |

## Rules

1. Frames are real Ethernet bytes in guest RAM; the NIC copies only validated DMA ranges.
2. ARP replies are built from the parsed request and local MAC/IPv4 — not canned stubs.
3. Host inject (`haeun_vm_net_inject`) delivers a frame into the guest RX DMA buffer; TX kicks invoke the host TX sink with the guest buffer bytes.
4. The stack (`haeun_net_poll`) reads RX from guest RAM through the NIC STATUS/RX_LEN path.

## NIC registers (`0x10007000`)

| Offset | Register | Behavior |
|---:|---|---|
| 0x00 | STATUS | bit0 RX pending |
| 0x04 | CONTROL | bit0 enable, bit1 RX IRQ → IRQC source 5 |
| 0x08 | MAC0 | MAC bytes 0..3 |
| 0x0C | MAC1 | MAC bytes 4..5 |
| 0x10 | TX_ADDR | guest PA |
| 0x14 | TX_LEN | bytes |
| 0x18 | TX_CMD | write 1 → transmit |
| 0x1C | RX_ADDR | guest PA |
| 0x20 | RX_CAP | capacity |
| 0x24 | RX_LEN | length of pending frame |
| 0x28 | RX_ACK | write 1 → clear RX pending |

## Tests

`tests/net_tests.c` covers TX capture, inject→RX DMA, and ARP reply bytes through the NIC.

## Next networking milestone

Completed in v2 (IPv4/ICMP). Next: UDP over IPv4.
