#include "haeun/evt.h"

#include <stdio.h>
#include <string.h>

static void evt_log(const char *detail) {
    (void)fprintf(stderr, "[haeun-evt] %s\n", detail != NULL ? detail : "");
}

static haeun_evt_slot *slot_for(haeun_evt_hub *hub, int pid, int create) {
    int i;
    int free_i = -1;
    if (hub == NULL || pid <= 0)
        return NULL;
    for (i = 0; i < HAEUN_EVT_SLOTS; ++i) {
        if (hub->slots[i].owner_pid == pid)
            return &hub->slots[i];
        if (free_i < 0 && hub->slots[i].owner_pid == 0)
            free_i = i;
    }
    if (!create || free_i < 0)
        return NULL;
    hub->slots[free_i].owner_pid = pid;
    hub->slots[free_i].head = 0u;
    hub->slots[free_i].count = 0u;
    hub->slots[free_i].drops = 0u;
    return &hub->slots[free_i];
}

/** Clears all queues; optional wm for focus routing. */
void haeun_evt_hub_init(haeun_evt_hub *hub, haeun_gfx_wm *wm) {
    if (hub == NULL)
        return;
    memset(hub, 0, sizeof(*hub));
    hub->wm = wm;
    hub->last_focus = -1;
}

/** Drops every event owned by pid (process exit / fault). */
void haeun_evt_reap(haeun_evt_hub *hub, int pid) {
    int i;
    if (hub == NULL || pid <= 0)
        return;
    for (i = 0; i < HAEUN_EVT_SLOTS; ++i) {
        if (hub->slots[i].owner_pid == pid) {
            memset(&hub->slots[i], 0, sizeof(hub->slots[i]));
        }
    }
}

/**
 * Enqueues an event for pid. On overflow drops the oldest and counts a drop.
 */
int haeun_evt_push(haeun_evt_hub *hub, int pid, const haeun_evt *ev) {
    haeun_evt_slot *s;
    uint8_t idx;
    if (hub == NULL || ev == NULL || pid <= 0)
        return -1;
    s = slot_for(hub, pid, 1);
    if (s == NULL) {
        evt_log("no event slot");
        return -1;
    }
    if (s->count == (uint8_t)HAEUN_EVT_QUEUE) {
        s->head = (uint8_t)((s->head + 1u) % HAEUN_EVT_QUEUE);
        s->count--;
        s->drops++;
        evt_log("queue overflow (drop oldest)");
    }
    idx = (uint8_t)((s->head + s->count) % HAEUN_EVT_QUEUE);
    s->q[idx] = *ev;
    s->count++;
    return 0;
}

/** Non-blocking pop for pid; returns 1 and fills *out, or 0 if empty. */
int haeun_evt_poll(haeun_evt_hub *hub, int pid, haeun_evt *out) {
    haeun_evt_slot *s;
    if (hub == NULL || out == NULL || pid <= 0)
        return 0;
    s = slot_for(hub, pid, 0);
    if (s == NULL || s->count == 0u)
        return 0;
    *out = s->q[s->head];
    s->head = (uint8_t)((s->head + 1u) % HAEUN_EVT_QUEUE);
    s->count--;
    return 1;
}

static int owner_of(haeun_evt_hub *hub, int win) {
    if (hub == NULL || hub->wm == NULL || win < 0 ||
        win >= HAEUN_GFX_WM_MAX_WINDOWS || !hub->wm->windows[win].used)
        return 0;
    return hub->wm->windows[win].owner_pid;
}

/** True when type is a window lifecycle/focus event (not input). */
int haeun_evt_is_window(uint16_t type) {
    if (type >= (uint16_t)HAEUN_EVT_FOCUS && type <= (uint16_t)HAEUN_EVT_RESIZE)
        return 1;
    if (type >= (uint16_t)HAEUN_EVT_CREATED && type <= (uint16_t)HAEUN_EVT_RESTORED)
        return 1;
    if (type == (uint16_t)HAEUN_EVT_DESTROYED)
        return 1;
    return 0;
}

/** Posts a window event to the window owner's queue (no-op if unowned). */
void haeun_evt_win_post(haeun_evt_hub *hub, int win, uint16_t type, uint32_t a,
                        uint32_t b) {
    haeun_evt ev;
    int pid;
    if (hub == NULL || win < 0)
        return;
    pid = owner_of(hub, win);
    if (pid <= 0)
        return;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.win = (int16_t)win;
    ev.a = a;
    ev.b = b;
    if (haeun_evt_push(hub, pid, &ev) != 0)
        evt_log("win_post failed");
}

/** True when focus_id names a used+visible+!minimized window. */
static int focus_eligible(const haeun_evt_hub *hub, int win) {
    const haeun_gfx_window *w;
    if (hub == NULL || hub->wm == NULL || win < 0 ||
        win >= HAEUN_GFX_WM_MAX_WINDOWS)
        return 0;
    w = &hub->wm->windows[win];
    return w->used && w->visible && !w->minimized;
}

/** Pushes KEY_DOWN/UP to the focused eligible window's owner_pid. */
void haeun_evt_route_key(haeun_evt_hub *hub, uint8_t scancode, int is_break) {
    haeun_evt ev;
    int win;
    int pid;
    if (hub == NULL || hub->wm == NULL)
        return;
    win = hub->wm->focus_id;
    if (!focus_eligible(hub, win)) {
        evt_log("key dropped (ineligible focus)");
        return;
    }
    pid = owner_of(hub, win);
    if (pid <= 0) {
        evt_log("key dropped (no focused owner)");
        return;
    }
    memset(&ev, 0, sizeof(ev));
    ev.type = is_break ? (uint16_t)HAEUN_EVT_KEY_UP : (uint16_t)HAEUN_EVT_KEY_DOWN;
    ev.win = (int16_t)win;
    ev.a = scancode & 0x7fu;
    (void)fprintf(stderr, "[haeun-evt] key pid=%d sc=0x%02x %s\n", pid,
                  (unsigned)(scancode & 0x7fu),
                  is_break ? "up" : "down");
    if (haeun_evt_push(hub, pid, &ev) != 0)
        evt_log("key push failed");
}

/** Emits FOCUS/BLUR when focus_id changes. */
void haeun_evt_on_focus(haeun_evt_hub *hub, int new_focus_id) {
    haeun_evt ev;
    int old;
    int pid;
    if (hub == NULL)
        return;
    old = hub->last_focus;
    if (old == new_focus_id)
        return;
    if (old >= 0) {
        pid = owner_of(hub, old);
        if (pid > 0) {
            memset(&ev, 0, sizeof(ev));
            ev.type = (uint16_t)HAEUN_EVT_BLUR;
            ev.win = (int16_t)old;
            (void)haeun_evt_push(hub, pid, &ev);
        }
    }
    if (new_focus_id >= 0) {
        pid = owner_of(hub, new_focus_id);
        if (pid > 0) {
            memset(&ev, 0, sizeof(ev));
            ev.type = (uint16_t)HAEUN_EVT_FOCUS;
            ev.win = (int16_t)new_focus_id;
            (void)haeun_evt_push(hub, pid, &ev);
        }
    }
    hub->last_focus = new_focus_id;
}

/**
 * Aligns hub->last_focus with wm->focus_id.
 * Mutation paths (minimize/destroy/hide) must recover focus first; this only
 * clears stale focus_id when the pointed slot is no longer eligible.
 */
void haeun_evt_sync_focus(haeun_evt_hub *hub) {
    int cur;
    if (hub == NULL || hub->wm == NULL)
        return;
    cur = hub->wm->focus_id;
    if (cur >= 0) {
        const haeun_gfx_window *w;
        if (cur >= HAEUN_GFX_WM_MAX_WINDOWS) {
            cur = -1;
        } else {
            w = &hub->wm->windows[cur];
            if (!w->used || !w->visible || w->minimized)
                cur = -1;
        }
        if (cur != hub->wm->focus_id)
            hub->wm->focus_id = cur;
    }
    haeun_evt_on_focus(hub, cur);
}

/** Client-area mouse event for focused window (screen coords). */
void haeun_evt_route_mouse(haeun_evt_hub *hub, int32_t sx, int32_t sy,
                           uint32_t buttons, uint32_t prev_buttons) {
    haeun_evt ev;
    haeun_gfx_window *w;
    int win;
    int pid;
    int32_t lx, ly;
    uint32_t down = buttons & 7u;
    uint32_t prev = prev_buttons & 7u;
    int moved;
    if (hub == NULL || hub->wm == NULL)
        return;
    win = hub->wm->focus_id;
    if (!focus_eligible(hub, win))
        return;
    pid = owner_of(hub, win);
    if (pid <= 0)
        return;
    w = &hub->wm->windows[win];
    lx = sx - w->x;
    ly = sy - w->y;
    if (lx < 0 || ly < 0 || (uint32_t)lx >= w->w || (uint32_t)ly >= w->h)
        return;
    moved = !hub->have_mouse || sx != hub->last_mx || sy != hub->last_my;
    hub->last_mx = sx;
    hub->last_my = sy;
    hub->have_mouse = 1;
    memset(&ev, 0, sizeof(ev));
    ev.win = (int16_t)win;
    ev.a = (uint32_t)lx;
    ev.b = (uint32_t)ly;
    if ((down & ~prev) != 0u)
        ev.type = (uint16_t)HAEUN_EVT_MOUSE_DOWN;
    else if ((prev & ~down) != 0u)
        ev.type = (uint16_t)HAEUN_EVT_MOUSE_UP;
    else if (moved || down != 0u)
        ev.type = (uint16_t)HAEUN_EVT_MOUSE_MOVE;
    else
        return;
    if (haeun_evt_push(hub, pid, &ev) != 0)
        evt_log("mouse push failed");
}

/** Wheel delta for focused window client area. */
void haeun_evt_route_wheel(haeun_evt_hub *hub, int32_t sx, int32_t sy,
                           int32_t delta) {
    haeun_evt ev;
    haeun_gfx_window *w;
    int win;
    int pid;
    int32_t lx, ly;
    if (hub == NULL || hub->wm == NULL || delta == 0)
        return;
    win = hub->wm->focus_id;
    if (!focus_eligible(hub, win)) {
        evt_log("wheel dropped (ineligible focus)");
        return;
    }
    pid = owner_of(hub, win);
    if (pid <= 0) {
        evt_log("wheel dropped (no focused owner)");
        return;
    }
    w = &hub->wm->windows[win];
    lx = sx - w->x;
    ly = sy - w->y;
    if (lx < 0 || ly < 0 || (uint32_t)lx >= w->w || (uint32_t)ly >= w->h)
        return;
    memset(&ev, 0, sizeof(ev));
    ev.type = (uint16_t)HAEUN_EVT_MOUSE_WHEEL;
    ev.win = (int16_t)win;
    ev.a = (uint32_t)lx;
    ev.b = (uint32_t)(int32_t)delta;
    if (haeun_evt_push(hub, pid, &ev) != 0)
        evt_log("wheel push failed");
}
