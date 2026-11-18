#include "haeun/vm_input_host.h"

#include <string.h>

/* XT set-1 makes used by guest apps / desktop (documented in app_*.s). */
enum {
    SC_ESC = 0x01u,
    SC_1 = 0x02u,
    SC_2 = 0x03u,
    SC_3 = 0x04u,
    SC_4 = 0x05u,
    SC_5 = 0x06u,
    SC_6 = 0x07u,
    SC_7 = 0x08u,
    SC_8 = 0x09u,
    SC_9 = 0x0Au,
    SC_0 = 0x0Bu,
    SC_MINUS = 0x0Cu,
    SC_EQUALS = 0x0Du, /* + with shift omitted; calc treats as add */
    SC_BS = 0x0Eu,
    SC_A = 0x1Eu,
    SC_B = 0x30u,
    SC_ENTER = 0x1Cu,
    SC_SPACE = 0x39u,
    SC_X = 0x2Du, /* calc multiply */
    SC_SLASH = 0x35u
};

static void reset_script(haeun_vm_input_host *host) {
    host->event_count = 0;
    host->next_event = 0;
}

static void push_key(haeun_vm_input_host *host, uint64_t at, uint8_t sc) {
    if (host->event_count >= HAEUN_INPUT_HOST_MAX_EVENTS)
        return;
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = at, .kind = HAEUN_INPUT_EV_KEY, .scancode = sc};
}

static void push_click(haeun_vm_input_host *host, uint64_t at, int32_t x,
                       int32_t y) {
    if (host->event_count + 1 >= HAEUN_INPUT_HOST_MAX_EVENTS)
        return;
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = at,
        .kind = HAEUN_INPUT_EV_MOUSE,
        .x = x,
        .y = y,
        .buttons = 1u};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = at + 100u,
        .kind = HAEUN_INPUT_EV_MOUSE,
        .x = x,
        .y = y,
        .buttons = 0u};
}

/** Focus shell header so desktop receives spawn keys. */
static void focus_shell(haeun_vm_input_host *host, uint64_t at) {
    push_click(host, at, 160, 10);
}

/** Focus calc client area (window at 8,18 size 148x64). */
static void focus_calc(haeun_vm_input_host *host, uint64_t at) {
    push_click(host, at, 80, 50);
}

/** Focus edit client area (window at 164,18 size 148x64). */
static void focus_edit(haeun_vm_input_host *host, uint64_t at) {
    push_click(host, at, 238, 50);
}

/** Focus files client area (window at 80,18 size 160x64). */
static void focus_files(haeun_vm_input_host *host, uint64_t at) {
    push_click(host, at, 160, 50);
}

static void spawn_app(haeun_vm_input_host *host, uint8_t digit_sc) {
    focus_shell(host, 2000u);
    push_key(host, 8000u, digit_sc);
}

static void load_calc_expr(haeun_vm_input_host *host, uint8_t d0, uint8_t op,
                           uint8_t d1) {
    reset_script(host);
    spawn_app(host, SC_1);
    focus_calc(host, 45000u);
    push_key(host, 50000u, d0);
    push_key(host, 52000u, op);
    push_key(host, 54000u, d1);
    push_key(host, 56000u, SC_ENTER);
}

/**
 * Loads a named M22.3 interactive desktop input script.
 */
int haeun_vm_input_host_load_m22_3_script(haeun_vm_input_host *host,
                                          const char *name) {
    if (host == NULL || name == NULL)
        return -1;
    if (strcmp(name, "calc-add") == 0) {
        load_calc_expr(host, SC_1, SC_EQUALS, SC_2); /* 1+2= */
        return 0;
    }
    if (strcmp(name, "calc-sub") == 0) {
        load_calc_expr(host, SC_9, SC_MINUS, SC_4); /* 9-4= */
        return 0;
    }
    if (strcmp(name, "calc-mul") == 0) {
        load_calc_expr(host, SC_2, SC_X, SC_2); /* 2x2= */
        return 0;
    }
    if (strcmp(name, "calc-div") == 0) {
        load_calc_expr(host, SC_8, SC_SLASH, SC_2); /* 8/2= */
        return 0;
    }
    if (strcmp(name, "calc-bs") == 0) {
        reset_script(host);
        spawn_app(host, SC_1);
        focus_calc(host, 45000u);
        push_key(host, 50000u, SC_1);
        push_key(host, 52000u, SC_2);
        push_key(host, 54000u, SC_BS);
        return 0;
    }
    if (strcmp(name, "calc-esc") == 0) {
        reset_script(host);
        spawn_app(host, SC_1);
        focus_calc(host, 45000u);
        push_key(host, 50000u, SC_5);
        push_key(host, 60000u, SC_ESC); /* close calc */
        focus_shell(host, 120000u);
        push_key(host, 160000u, SC_ESC); /* shutdown desktop */
        return 0;
    }
    if (strcmp(name, "edit-type") == 0) {
        reset_script(host);
        spawn_app(host, SC_2);
        focus_edit(host, 45000u);
        push_key(host, 50000u, SC_A);
        push_key(host, 52000u, SC_B);
        push_key(host, 54000u, SC_SPACE);
        push_key(host, 56000u, SC_1);
        push_key(host, 58000u, SC_BS);
        return 0;
    }
    if (strcmp(name, "files-list") == 0) {
        reset_script(host);
        spawn_app(host, SC_3);
        focus_files(host, 45000u);
        return 0;
    }
    if (strcmp(name, "files-esc") == 0) {
        reset_script(host);
        spawn_app(host, SC_3);
        focus_files(host, 45000u);
        push_key(host, 80000u, SC_ESC);
        focus_shell(host, 140000u);
        push_key(host, 180000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "solo-esc") == 0) {
        /* For --app smoke guests that wait on Esc (e.g. filemgr). */
        reset_script(host);
        push_key(host, 50000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "chrome-close-calc") == 0) {
        /* Spawn calc, click chrome Close (not Esc), then Esc desktop. */
        reset_script(host);
        spawn_app(host, SC_1);
        focus_calc(host, 45000u);
        /* Close: calc (8,18,148x64) → bx=151, ty=12; click (153,14). */
        push_click(host, 70000u, 153, 14);
        focus_shell(host, 140000u);
        push_key(host, 180000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "chrome-close-app") == 0) {
        /* Solo guest window at calc geometry; chrome Close only. */
        reset_script(host);
        push_click(host, 40000u, 153, 14);
        return 0;
    }
    if (strcmp(name, "m23-focus-min") == 0) {
        /* Two apps; minimize focused calc → focus recovers to edit. */
        reset_script(host);
        spawn_app(host, SC_1);
        focus_calc(host, 40000u);
        focus_shell(host, 70000u);
        push_key(host, 90000u, SC_2);
        focus_edit(host, 130000u);
        focus_calc(host, 160000u);
        /* Min: calc bx=146, ty=12; click (148,14). */
        push_click(host, 190000u, 148, 14);
        focus_shell(host, 260000u);
        push_key(host, 300000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "m23-input-route") == 0) {
        /*
         * Concurrent calc+edit. Side-by-side client hits:
         * calc-only (80,50); edit-only (238,50).
         */
        reset_script(host);
        spawn_app(host, SC_1);
        push_click(host, 45000u, 80, 50);
        push_key(host, 55000u, SC_A);
        focus_shell(host, 70000u);
        push_key(host, 90000u, SC_2);
        push_click(host, 130000u, 238, 50);
        push_key(host, 150000u, SC_B);
        push_click(host, 180000u, 80, 50);
        push_key(host, 200000u, SC_A);
        push_click(host, 220000u, 80, 50);
        push_click(host, 240000u, 148, 14); /* minimize calc */
        push_click(host, 260000u, 238, 50); /* edit after recover */
        push_key(host, 280000u, SC_B);
        focus_shell(host, 320000u);
        push_key(host, 360000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "m23-redraw") == 0) {
        /* Raise / min for PPM; non-overlapping focus clicks. */
        reset_script(host);
        spawn_app(host, SC_1);
        push_click(host, 45000u, 80, 50);
        focus_shell(host, 70000u);
        push_key(host, 90000u, SC_2);
        push_click(host, 130000u, 238, 50); /* raise edit */
        push_click(host, 170000u, 80, 50);   /* raise calc */
        push_click(host, 200000u, 148, 14);  /* min calc */
        push_click(host, 230000u, 238, 50);
        focus_shell(host, 270000u);
        push_key(host, 320000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "m23-stab-a") == 0) {
        /* Integrated: calc+edit, raise, keys, min, recover, chrome close. */
        reset_script(host);
        spawn_app(host, SC_1);
        push_click(host, 45000u, 80, 50);
        push_key(host, 55000u, SC_A);
        focus_shell(host, 70000u);
        push_key(host, 90000u, SC_2);
        push_click(host, 130000u, 238, 50);
        push_key(host, 150000u, SC_B);
        push_click(host, 170000u, 80, 50);
        push_key(host, 190000u, SC_A);
        push_click(host, 210000u, 238, 50);
        push_key(host, 230000u, SC_B);
        push_click(host, 250000u, 80, 50);
        push_click(host, 270000u, 148, 14); /* min calc */
        push_click(host, 290000u, 238, 50);
        push_key(host, 310000u, SC_B); /* edit only after recover */
        push_click(host, 330000u, 238, 50);
        /* edit Close: (164,18,148x64) → bx=307; click (309,14). */
        push_click(host, 350000u, 309, 14);
        focus_shell(host, 420000u);
        push_key(host, 480000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "m23-stab-b") == 0) {
        /* Chrome CLOSE focused calc; desktop survives until Esc. */
        reset_script(host);
        spawn_app(host, SC_1);
        push_click(host, 45000u, 80, 50);
        push_key(host, 55000u, SC_7);
        push_click(host, 70000u, 153, 14); /* chrome Close */
        focus_shell(host, 140000u);
        push_key(host, 200000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "m23-stab-stress") == 0) {
        /* Rapid focus switch + keys between calc and edit. */
        reset_script(host);
        spawn_app(host, SC_1);
        focus_shell(host, 40000u);
        push_key(host, 60000u, SC_2);
        push_click(host, 100000u, 80, 50);
        push_key(host, 110000u, SC_A);
        push_click(host, 120000u, 238, 50);
        push_key(host, 130000u, SC_B);
        push_click(host, 140000u, 80, 50);
        push_key(host, 150000u, SC_A);
        push_click(host, 160000u, 238, 50);
        push_key(host, 170000u, SC_B);
        push_click(host, 180000u, 80, 50);
        push_key(host, 190000u, SC_A);
        push_click(host, 200000u, 238, 50);
        push_key(host, 210000u, SC_B);
        push_click(host, 230000u, 80, 50);
        push_click(host, 250000u, 148, 14);
        push_click(host, 270000u, 238, 50);
        push_key(host, 290000u, SC_B);
        focus_shell(host, 340000u);
        push_key(host, 400000u, SC_ESC);
        return 0;
    }
    if (strcmp(name, "life-cycle") == 0) {
        reset_script(host);
        focus_shell(host, 2000u);
        push_key(host, 8000u, SC_1);
        focus_calc(host, 40000u);
        push_key(host, 50000u, SC_7);
        push_key(host, 70000u, SC_ESC);
        focus_shell(host, 110000u);
        push_key(host, 130000u, SC_2);
        focus_edit(host, 170000u);
        push_key(host, 190000u, SC_A);
        push_key(host, 210000u, SC_ESC);
        focus_shell(host, 250000u);
        push_key(host, 270000u, SC_3);
        focus_files(host, 310000u);
        push_key(host, 340000u, SC_ESC);
        focus_shell(host, 390000u);
        push_key(host, 420000u, SC_1);
        push_key(host, 460000u, SC_ESC);
        focus_shell(host, 520000u);
        push_key(host, 560000u, SC_ESC);
        return 0;
    }
    return -1;
}
