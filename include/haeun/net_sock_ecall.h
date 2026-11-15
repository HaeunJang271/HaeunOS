#ifndef HAEUN_NET_SOCK_ECALL_H
#define HAEUN_NET_SOCK_ECALL_H

#include "haeun/net_sock.h"
#include "haeun/vm.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    haeun_net_socks *ss;
    uint8_t next_hop_mac[6];
} haeun_sock_ecall_ctx;

/**
 * Host ECALL assist for SOCKET/BIND/SENDTO/RECVFROM/LISTEN/ACCEPT/SO_CLOSE.
 * SENDTO: r2=fd r3=dst_ip r4=dst_port r5=buf_va r6=len (MAC from ctx).
 * RECVFROM: r2=fd r3=buf_va r4=cap; returns length in r1, writes src_ip/port
 * to guest words at r5 (optional; 0 skips).
 */
bool haeun_sock_ecall_io(haeun_vm *vm, void *context);

#endif
