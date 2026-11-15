#include "haeun/ipc.h"

#include <stdio.h>
#include <string.h>

static void ipc_log(const char *detail) {
    (void)fprintf(stderr, "[haeun-ipc] %s\n", detail != NULL ? detail : "");
}

/** Clears the IPC table. */
void haeun_ipc_hub_init(haeun_ipc_hub *hub) {
    if (hub == NULL)
        return;
    memset(hub, 0, sizeof(*hub));
}

/** Closes every endpoint owned by pid (EXIT / fault). */
void haeun_ipc_reap(haeun_ipc_hub *hub, int pid) {
    int i;
    if (hub == NULL || pid <= 0)
        return;
    for (i = 0; i < HAEUN_IPC_MAX_ENDPOINTS; ++i) {
        if (hub->eps[i].used && hub->eps[i].owner_pid == pid)
            memset(&hub->eps[i], 0, sizeof(hub->eps[i]));
    }
}

/** Creates an endpoint owned by pid; returns id or -1. */
int haeun_ipc_create(haeun_ipc_hub *hub, int pid) {
    int i;
    if (hub == NULL || pid <= 0)
        return -1;
    for (i = 0; i < HAEUN_IPC_MAX_ENDPOINTS; ++i) {
        if (!hub->eps[i].used) {
            memset(&hub->eps[i], 0, sizeof(hub->eps[i]));
            hub->eps[i].used = 1;
            hub->eps[i].owner_pid = pid;
            return i;
        }
    }
    ipc_log("endpoint table full");
    return -1;
}

/** Owner-only close; returns 0 or -1. */
int haeun_ipc_close(haeun_ipc_hub *hub, int epid, int pid) {
    if (hub == NULL || epid < 0 || epid >= HAEUN_IPC_MAX_ENDPOINTS)
        return -1;
    if (!hub->eps[epid].used || hub->eps[epid].owner_pid != pid) {
        ipc_log("close ownership denied");
        return -1;
    }
    memset(&hub->eps[epid], 0, sizeof(hub->eps[epid]));
    return 0;
}

/**
 * Enqueues a message. sender_pid is trusted only from the kernel caller.
 */
int haeun_ipc_send(haeun_ipc_hub *hub, int epid, int sender_pid,
                   const uint8_t *data, uint32_t len) {
    haeun_ipc_endpoint *e;
    haeun_ipc_msg *m;
    uint8_t idx;
    if (hub == NULL || data == NULL || sender_pid <= 0)
        return -1;
    if (epid < 0 || epid >= HAEUN_IPC_MAX_ENDPOINTS || !hub->eps[epid].used)
        return -1;
    if (len == 0u || len > (uint32_t)HAEUN_IPC_MSG_MAX)
        return -3;
    e = &hub->eps[epid];
    if (e->owner_pid <= 0) {
        ipc_log("send to inactive owner");
        return -1;
    }
    if (e->count >= (uint8_t)HAEUN_IPC_QUEUE) {
        e->drops++;
        ipc_log("queue full");
        return -2;
    }
    idx = (uint8_t)((e->head + e->count) % HAEUN_IPC_QUEUE);
    m = &e->q[idx];
    memset(m, 0, sizeof(*m));
    memcpy(m->data, data, len);
    m->len = (uint8_t)len;
    m->type = data[0];
    m->seq = (uint16_t)(e->next_seq++);
    m->sender_pid = sender_pid;
    m->receiver_pid = e->owner_pid;
    e->count++;
    return 0;
}

/**
 * Owner-only non-blocking receive.
 */
int haeun_ipc_recv(haeun_ipc_hub *hub, int epid, int pid, uint8_t *out,
                   uint32_t cap, int *sender_pid) {
    haeun_ipc_endpoint *e;
    haeun_ipc_msg *m;
    uint32_t n;
    if (hub == NULL || out == NULL || pid <= 0)
        return -1;
    if (epid < 0 || epid >= HAEUN_IPC_MAX_ENDPOINTS || !hub->eps[epid].used)
        return -1;
    e = &hub->eps[epid];
    if (e->owner_pid != pid) {
        ipc_log("recv ownership denied");
        return -1;
    }
    if (e->count == 0u)
        return 0;
    m = &e->q[e->head];
    n = m->len;
    if (n > cap)
        n = cap;
    memcpy(out, m->data, n);
    if (sender_pid != NULL)
        *sender_pid = m->sender_pid;
    e->head = (uint8_t)((e->head + 1u) % HAEUN_IPC_QUEUE);
    e->count--;
    return (int)n;
}

/** Owner-only queued message count. */
int haeun_ipc_available(haeun_ipc_hub *hub, int epid, int pid) {
    haeun_ipc_endpoint *e;
    if (hub == NULL || pid <= 0)
        return -1;
    if (epid < 0 || epid >= HAEUN_IPC_MAX_ENDPOINTS || !hub->eps[epid].used)
        return -1;
    e = &hub->eps[epid];
    if (e->owner_pid != pid) {
        ipc_log("available ownership denied");
        return -1;
    }
    return (int)e->count;
}

/** Owner-only peek at head metadata without dequeue. */
int haeun_ipc_peek(haeun_ipc_hub *hub, int epid, int pid, haeun_ipc_msg *out) {
    haeun_ipc_endpoint *e;
    if (hub == NULL || out == NULL || pid <= 0)
        return -1;
    if (epid < 0 || epid >= HAEUN_IPC_MAX_ENDPOINTS || !hub->eps[epid].used)
        return -1;
    e = &hub->eps[epid];
    if (e->owner_pid != pid) {
        ipc_log("peek ownership denied");
        return -1;
    }
    if (e->count == 0u)
        return 0;
    *out = e->q[e->head];
    return 1;
}
