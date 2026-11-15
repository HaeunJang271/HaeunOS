#ifndef HAEUN_SVC_H
#define HAEUN_SVC_H

#include <stdint.h>

enum { HAEUN_SVC_MAX = 8, HAEUN_SVC_NAME = 12 };

typedef struct {
    int used;
    int owner_pid;
    int epid;
    char name[HAEUN_SVC_NAME];
} haeun_svc_entry;

typedef struct {
    haeun_svc_entry tab[HAEUN_SVC_MAX];
} haeun_svc_hub;

/** Clears the service registry. */
void haeun_svc_hub_init(haeun_svc_hub *hub);

/** Drops registrations owned by pid. */
void haeun_svc_reap(haeun_svc_hub *hub, int pid);

/** Registers name→epid for owner; returns 0 or -1. */
int haeun_svc_register(haeun_svc_hub *hub, int pid, const char *name, int epid);

/** Looks up name; returns epid or -1. */
int haeun_svc_lookup(const haeun_svc_hub *hub, const char *name);

#endif
