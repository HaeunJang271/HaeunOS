#include "haeun/fb_view.h"
#include "haeun/vm_gui_host.h"
#include "haeun/vm_viewer_plat.h"

#include <stdlib.h>
#include <string.h>

enum { VIEW_SLICE = 256u, VIEW_BGRA_MAX = 1024u * 1024u * 4u };

static viewer_plat *g_plat;

static void feed_key(haeun_vm_viewer *v, uint8_t code) {
    if (v == NULL || v->vm == NULL)
        return;
    if (haeun_vm_keyboard_feed(v->vm, &code, 1u))
        v->keys_fed++;
    if (v->chrome != NULL)
        (void)haeun_gfx_chrome_key(v->chrome, code);
}

static void post_mouse(haeun_vm_viewer *v, int32_t x, int32_t y, uint32_t buttons) {
    int hit = -1;
    if (v == NULL || v->vm == NULL)
        return;
    if (!haeun_vm_mouse_post(v->vm, x, y, buttons))
        return;
    v->mice_posted++;
    v->mouse_buttons = buttons & 7u;
    if (v->chrome != NULL) {
        int prev = v->chrome->wm != NULL ? v->chrome->wm->focus_id : -1;
        (void)haeun_gfx_chrome_pointer(v->chrome, x, y, buttons);
        if (v->chrome->wm != NULL && v->chrome->wm->focus_id >= 0 &&
            v->chrome->wm->focus_id != prev)
            v->focus_hits++;
        return;
    }
    if (v->wm != NULL && (buttons & 1u) != 0u) {
        if (haeun_gfx_wm_hit_focus(v->wm, x, y, &hit) == HAEUN_GFX_OK &&
            hit >= 0)
            v->focus_hits++;
    }
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    viewer_plat *p = g_plat;
    haeun_vm_viewer *v = p != NULL ? p->viewer : NULL;
    if (msg == WM_DESTROY) {
        if (p != NULL)
            p->quit = 1;
        PostQuitMessage(0);
        return 0;
    }
    if (v == NULL || !v->live_input || v->vm == NULL)
        return DefWindowProc(hwnd, msg, wp, lp);
    if (msg == WM_KEYDOWN || msg == WM_KEYUP) {
        UINT sc = MapVirtualKey((UINT)wp, MAPVK_VK_TO_VSC) & 0x7Fu;
        uint8_t code = (uint8_t)sc;
        if (msg == WM_KEYUP)
            code = (uint8_t)(code | 0x80u);
        if (sc != 0u)
            feed_key(v, code);
        return 0;
    }
    if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
        msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
        msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {
        RECT rc;
        int32_t fx = 0, fy = 0;
        uint32_t buttons = 0u;
        GetClientRect(hwnd, &rc);
        if ((wp & MK_LBUTTON) != 0u)
            buttons |= 1u;
        if ((wp & MK_RBUTTON) != 0u)
            buttons |= 2u;
        if ((wp & MK_MBUTTON) != 0u)
            buttons |= 4u;
        if (haeun_fb_view_map_mouse((int)(short)LOWORD(lp), (int)(short)HIWORD(lp),
                                    rc.right - rc.left, rc.bottom - rc.top,
                                    p->fb_w != 0u ? p->fb_w : HAEUN_GUI_FB_WIDTH,
                                    p->fb_h != 0u ? p->fb_h : HAEUN_GUI_FB_HEIGHT,
                                    &fx, &fy) == 0)
            post_mouse(v, fx, fy, buttons);
        return 0;
    }
    if (msg == WM_MOUSEWHEEL) {
        RECT rc;
        POINT pt;
        int32_t fx = 0, fy = 0;
        short delta = (short)HIWORD(wp);
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        GetClientRect(hwnd, &rc);
        if (haeun_fb_view_map_mouse(pt.x, pt.y, rc.right - rc.left,
                                    rc.bottom - rc.top,
                                    p->fb_w != 0u ? p->fb_w : HAEUN_GUI_FB_WIDTH,
                                    p->fb_h != 0u ? p->fb_h : HAEUN_GUI_FB_HEIGHT,
                                    &fx, &fy) == 0) {
            if (v->chrome != NULL && v->chrome->evt != NULL)
                haeun_evt_route_wheel(v->chrome->evt, fx, fy, (int32_t)delta);
        }
        return 0;
    }
    if (msg == WM_ERASEBKGND)
        return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (p != NULL && p->fb_w != 0u)
            haeun_viewer_paint(p);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

static int open_window(viewer_plat *p, int scale) {
    WNDCLASSW wc;
    RECT cr;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"HaeunViewer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);
    cr.left = 0;
    cr.top = 0;
    cr.right = (LONG)HAEUN_GUI_FB_WIDTH * scale;
    cr.bottom = (LONG)HAEUN_GUI_FB_HEIGHT * scale;
    AdjustWindowRect(&cr, style & ~WS_VISIBLE, FALSE);
    p->hwnd = CreateWindowExW(0, L"HaeunViewer", L"HAEUN-OS Viewer", style,
                              CW_USEDEFAULT, CW_USEDEFAULT, cr.right - cr.left,
                              cr.bottom - cr.top, NULL, NULL, wc.hInstance,
                              NULL);
    return p->hwnd != NULL ? 0 : -1;
}

static void pump(viewer_plat *p) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            p->quit = 1;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

/** Arms the Win32 viewer; IRQ lines stay clear for poll-only M10 input. */
haeun_viewer_result haeun_vm_viewer_attach(haeun_vm_viewer *viewer, haeun_vm *vm,
                                           haeun_gfx_wm *wm, int live_input) {
    if (viewer == NULL || vm == NULL)
        return HAEUN_VIEWER_BAD_ARGUMENT;
    memset(viewer, 0, sizeof(*viewer));
    viewer->vm = vm;
    viewer->wm = wm;
    viewer->live_input = live_input != 0 ? 1 : 0;
    viewer->scale = 2; /* 320x180 → ~640x360 client */
    vm->kbd_control = 0u;
    vm->mouse_control = 0u;
    viewer->attached = 1;
    return HAEUN_VIEWER_OK;
}

static void apply_script(haeun_vm_viewer *viewer, uint64_t steps) {
    haeun_vm_input_host *s = viewer->script;
    if (s == NULL)
        return;
    while (s->next_event < s->event_count &&
           s->events[s->next_event].at_step <= steps) {
        const haeun_input_script_ev *ev = &s->events[s->next_event];
        if (ev->kind == HAEUN_INPUT_EV_KEY) {
            uint8_t b = ev->scancode;
            feed_key(viewer, b);
            s->keys_fed++;
        } else if (ev->kind == HAEUN_INPUT_EV_MOUSE) {
            post_mouse(viewer, ev->x, ev->y, ev->buttons);
            s->mice_posted++;
        }
        s->next_event++;
    }
}

/** Cooperative run: VM slices, message pump, FB blit; no concurrent threads. */
haeun_vm_result haeun_vm_viewer_run(haeun_vm_viewer *viewer, haeun_vm *vm,
                                    uint64_t max_steps,
                                    uint64_t *steps_executed) {
    viewer_plat plat;
    haeun_vm_result st = HAEUN_VM_OK;
    if (viewer == NULL || vm == NULL || steps_executed == NULL)
        return HAEUN_VM_BAD_ARGUMENT;
    memset(&plat, 0, sizeof(plat));
    plat.viewer = viewer;
    plat.bgra_cap = VIEW_BGRA_MAX;
    plat.bgra = (uint8_t *)malloc(plat.bgra_cap);
    if (plat.bgra == NULL)
        return HAEUN_VM_BAD_ARGUMENT;
    g_plat = &plat;
    if (open_window(&plat, viewer->scale) != 0) {
        free(plat.bgra);
        g_plat = NULL;
        return HAEUN_VM_BAD_ARGUMENT;
    }
    viewer->window_open = 1;
    viewer->platform = plat.hwnd;
    *steps_executed = 0u;
    while (*steps_executed < max_steps && !plat.quit) {
        uint64_t i;
        apply_script(viewer, *steps_executed);
        for (i = 0u; i < VIEW_SLICE && *steps_executed < max_steps; ++i) {
            st = haeun_vm_step(vm);
            ++(*steps_executed);
            if (st == HAEUN_VM_HALTED || st != HAEUN_VM_OK)
                goto done;
        }
        pump(&plat);
        haeun_viewer_refresh_fb(&plat);
    }
    if (!plat.quit && st == HAEUN_VM_OK && *steps_executed >= max_steps)
        st = HAEUN_VM_STEP_LIMIT;
done:
    haeun_viewer_refresh_fb(&plat);
    if (plat.hwnd != NULL)
        DestroyWindow(plat.hwnd);
    pump(&plat);
    haeun_viewer_blit_fini(&plat);
    viewer->window_open = 0;
    viewer->platform = NULL;
    free(plat.bgra);
    g_plat = NULL;
    return st;
}
