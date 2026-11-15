#ifndef HAEUN_IPC_H
#define HAEUN_IPC_H

#include <stdint.h>

enum {
    HAEUN_IPC_MAX_ENDPOINTS = 8,
    HAEUN_IPC_QUEUE = 8,
    HAEUN_IPC_MSG_MAX = 32
};

/**
 * Host-side IPC message (separate from input evt queues).
 * Guest payload is data[0..len); type/seq/sender/receiver are kernel metadata.
 * Convention: data[0] may carry an app-level type byte (copied into type).
 */
typedef struct {
    uint8_t data[HAEUN_IPC_MSG_MAX];
    uint8_t len;
    uint8_t type;
    uint16_t seq;
    int sender_pid;
    int receiver_pid;
} haeun_ipc_msg;

typedef struct {
    int used;
    int owner_pid;
    haeun_ipc_msg q[HAEUN_IPC_QUEUE];
    uint8_t head;
    uint8_t count;
    uint32_t drops;
    uint32_t next_seq;
} haeun_ipc_endpoint;

typedef struct {
    haeun_ipc_endpoint eps[HAEUN_IPC_MAX_ENDPOINTS];
} haeun_ipc_hub;

/** Clears the IPC table. */
void haeun_ipc_hub_init(haeun_ipc_hub *hub);

/** Closes every endpoint owned by pid (EXIT / fault). */
void haeun_ipc_reap(haeun_ipc_hub *hub, int pid);

/** Creates an endpoint owned by pid; returns id or -1. */
int haeun_ipc_create(haeun_ipc_hub *hub, int pid);

/** Owner-only close; returns 0 or -1. */
int haeun_ipc_close(haeun_ipc_hub *hub, int epid, int pid);

/**
 * Enqueues a message. sender_pid must be the kernel's current pid.
 * Returns 0, -1 bad id/args, -2 full (drops++), -3 bad len.
 */
int haeun_ipc_send(haeun_ipc_hub *hub, int epid, int sender_pid,
                   const uint8_t *data, uint32_t len);

/**
 * Owner-only non-blocking receive. Returns byte length, 0 if empty, -1 error.
 */
int haeun_ipc_recv(haeun_ipc_hub *hub, int epid, int pid, uint8_t *out,
                   uint32_t cap, int *sender_pid);

/**
 * Owner-only: number of queued messages, or -1 if not owner / bad id.
 */
int haeun_ipc_available(haeun_ipc_hub *hub, int epid, int pid);

/**
 * Owner-only peek at head metadata without dequeue.
 * Returns 1 if a message exists, 0 if empty, -1 on error.
 */
int haeun_ipc_peek(haeun_ipc_hub *hub, int epid, int pid, haeun_ipc_msg *out);

#endif
