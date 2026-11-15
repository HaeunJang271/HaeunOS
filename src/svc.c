#include "haeun/svc.h"

#include <stdio.h>
#include <string.h>

static void svc_log(const char *detail) {
    (void)fprintf(stderr, "[haeun-svc] %s\n", detail != NULL ? detail : "");
}

/** Clears the service registry. */
void haeun_svc_hub_init(haeun_svc_hub *hub) {
    if (hub == NULL)
        return;
    memset(hub, 0, sizeof(*hub));
}

/** Drops registrations owned by pid. */
void haeun_svc_reap(haeun_svc_hub *hub, int pid) {
    int i;
    if (hub == NULL || pid <= 0)
        return;
    for (i = 0; i < HAEUN_SVC_MAX; ++i) {
        if (hub->tab[i].used && hub->tab[i].owner_pid == pid)
            memset(&hub->tab[i], 0, sizeof(hub->tab[i]));
    }
}

/** Registers name→epid for owner; returns 0 or -1. */
int haeun_svc_register(haeun_svc_hub *hub, int pid, const char *name,
                       int epid) {
    int i;
    int free_i = -1;
    if (hub == NULL || name == NULL || pid <= 0 || epid < 0)
        return -1;
    if (name[0] == '\0')
        return -1;
    for (i = 0; i < HAEUN_SVC_MAX; ++i) {
        if (hub->tab[i].used &&
            strncmp(hub->tab[i].name, name, HAEUN_SVC_NAME) == 0) {
            svc_log("duplicate service name");
            return -1;
        }
        if (free_i < 0 && !hub->tab[i].used)
            free_i = i;
    }
    if (free_i < 0) {
        svc_log("service table full");
        return -1;
    }
    memset(&hub->tab[free_i], 0, sizeof(hub->tab[free_i]));
    hub->tab[free_i].used = 1;
    hub->tab[free_i].owner_pid = pid;
    hub->tab[free_i].epid = epid;
    (void)strncpy(hub->tab[free_i].name, name, HAEUN_SVC_NAME - 1);
    return 0;
}

/** Looks up name; returns epid or -1. */
int haeun_svc_lookup(const haeun_svc_hub *hub, const char *name) {
    int i;
    if (hub == NULL || name == NULL)
        return -1;
    for (i = 0; i < HAEUN_SVC_MAX; ++i) {
        if (hub->tab[i].used &&
            strncmp(hub->tab[i].name, name, HAEUN_SVC_NAME) == 0)
            return hub->tab[i].epid;
    }
    return -1;
}
