#ifndef HAEUN_EVT_H
#define HAEUN_EVT_H

#include "haeun/gfx_wm.h"

#include <stdint.h>

enum {
    HAEUN_EVT_NONE = 0,
    /* Input (M20.1) */
    HAEUN_EVT_KEY_DOWN = 1,
    HAEUN_EVT_KEY_UP = 2,
    HAEUN_EVT_MOUSE_MOVE = 3,
    HAEUN_EVT_MOUSE_DOWN = 4,
    HAEUN_EVT_MOUSE_UP = 5,
    /* Window (M20.4) — same queue, distinct type ids */
    HAEUN_EVT_FOCUS = 6,       /* WINDOW_FOCUSED */
    HAEUN_EVT_BLUR = 7,        /* WINDOW_UNFOCUSED */
    HAEUN_EVT_CLOSE = 8,       /* WINDOW_CLOSE_REQUEST */
    HAEUN_EVT_RESIZE = 9,      /* WINDOW_RESIZED (on gesture complete) */
    HAEUN_EVT_MOUSE_WHEEL = 10,
    HAEUN_EVT_CREATED = 11,    /* WINDOW_CREATED */
    HAEUN_EVT_MOVED = 12,      /* WINDOW_MOVED (on drag complete) */
    HAEUN_EVT_MINIMIZED = 13,
    HAEUN_EVT_MAXIMIZED = 14,
    HAEUN_EVT_RESTORED = 15,
    HAEUN_EVT_DESTROYED = 17,  /* 16 reserved historically as QUEUE capacity */
    HAEUN_EVT_QUEUE = 16,
    HAEUN_EVT_SLOTS = 8
};

/**
 * Fixed 12-byte guest-visible event (little-endian layout for register ABI).
 * type:u16, win:i16, a:u32, b:u32
 * KEY: a=scancode; mouse: a=lx b=ly; MOVE/RESIZE: a=x/w b=y/h.
 */
typedef struct {
    uint16_t type;
    int16_t win;
    uint32_t a;
    uint32_t b;
} haeun_evt;

typedef struct {
    haeun_evt q[HAEUN_EVT_QUEUE];
    uint8_t head;
    uint8_t count;
    uint32_t drops;
    int owner_pid;
} haeun_evt_slot;

typedef struct {
    haeun_evt_slot slots[HAEUN_EVT_SLOTS];
    haeun_gfx_wm *wm;
    int last_focus;
    int32_t last_mx;
    int32_t last_my;
    int have_mouse;
} haeun_evt_hub;

/** Clears all queues; optional wm for focus routing. */
void haeun_evt_hub_init(haeun_evt_hub *hub, haeun_gfx_wm *wm);

/** Drops every event owned by pid (process exit / fault). */
void haeun_evt_reap(haeun_evt_hub *hub, int pid);

/**
 * Enqueues an event for pid. On overflow drops the oldest and counts a drop.
 * Returns 0 on success, -1 if hub full (no slot for pid).
 */
int haeun_evt_push(haeun_evt_hub *hub, int pid, const haeun_evt *ev);

/** Non-blocking pop for pid; returns 1 and fills *out, or 0 if empty. */
int haeun_evt_poll(haeun_evt_hub *hub, int pid, haeun_evt *out);

/** True when type is a window lifecycle/focus event (not input). */
int haeun_evt_is_window(uint16_t type);

/**
 * Posts a window event to the window owner's queue (no-op if unowned).
 * a/b carry geometry when relevant (MOVED: x,y; RESIZED: w,h).
 */
void haeun_evt_win_post(haeun_evt_hub *hub, int win, uint16_t type, uint32_t a,
                        uint32_t b);

/** Pushes KEY_DOWN/UP to the focused window's owner_pid. */
void haeun_evt_route_key(haeun_evt_hub *hub, uint8_t scancode, int is_break);

/** Emits FOCUS/BLUR when focus_id changes. */
void haeun_evt_on_focus(haeun_evt_hub *hub, int new_focus_id);

/** Aligns hub->last_focus with wm->focus_id. */
void haeun_evt_sync_focus(haeun_evt_hub *hub);

/** Client-area mouse event for focused window (screen coords). */
void haeun_evt_route_mouse(haeun_evt_hub *hub, int32_t sx, int32_t sy,
                           uint32_t buttons, uint32_t prev_buttons);

/** Wheel delta for focused window client area. */
void haeun_evt_route_wheel(haeun_evt_hub *hub, int32_t sx, int32_t sy,
                           int32_t delta);

#endif
