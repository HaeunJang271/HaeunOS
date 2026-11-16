#include "haeun/fb_view.h"
#include "haeun/vm_viewer_plat.h"

#include <string.h>

/** Releases cached GDI backbuffer objects. */
void haeun_viewer_blit_fini(viewer_plat *p) {
    if (p == NULL)
        return;
    if (p->mem_dc != NULL && p->mem_old != NULL)
        SelectObject(p->mem_dc, p->mem_old);
    if (p->mem_bmp != NULL)
        DeleteObject(p->mem_bmp);
    if (p->mem_dc != NULL)
        DeleteDC(p->mem_dc);
    p->mem_dc = NULL;
    p->mem_bmp = NULL;
    p->mem_old = NULL;
    p->mem_w = 0;
    p->mem_h = 0;
}

static int ensure_backbuffer(viewer_plat *p, HDC hdc, int cw, int ch) {
    if (p->mem_dc != NULL && p->mem_w == cw && p->mem_h == ch)
        return 0;
    haeun_viewer_blit_fini(p);
    p->mem_dc = CreateCompatibleDC(hdc);
    p->mem_bmp = CreateCompatibleBitmap(hdc, cw, ch);
    if (p->mem_dc == NULL || p->mem_bmp == NULL) {
        haeun_viewer_blit_fini(p);
        return -1;
    }
    p->mem_old = (HBITMAP)SelectObject(p->mem_dc, p->mem_bmp);
    p->mem_w = cw;
    p->mem_h = ch;
    return 0;
}

/**
 * Double-buffered blit: compose off-screen, BitBlt once to the window.
 * Fixes maximize flicker from live-window black clears.
 */
void haeun_viewer_paint(viewer_plat *p) {
    BITMAPINFO bmi;
    HDC hdc;
    RECT rc;
    int scale, dw, dh, ox, oy, cw, ch;
    if (p == NULL || p->hwnd == NULL || p->bgra == NULL || p->fb_w == 0u)
        return;
    GetClientRect(p->hwnd, &rc);
    cw = rc.right - rc.left;
    ch = rc.bottom - rc.top;
    if (cw <= 0 || ch <= 0)
        return;
    scale = p->viewer != NULL ? p->viewer->scale : 8;
    dw = (int)p->fb_w * scale;
    dh = (int)p->fb_h * scale;
    ox = (cw - dw) / 2;
    oy = (ch - dh) / 2;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)p->fb_w;
    bmi.bmiHeader.biHeight = -(LONG)p->fb_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    hdc = GetDC(p->hwnd);
    if (hdc == NULL)
        return;
    if (ensure_backbuffer(p, hdc, cw, ch) != 0) {
        ReleaseDC(p->hwnd, hdc);
        return;
    }
    FillRect(p->mem_dc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    StretchDIBits(p->mem_dc, ox, oy, dw, dh, 0, 0, (int)p->fb_w, (int)p->fb_h,
                  p->bgra, &bmi, DIB_RGB_COLORS, SRCCOPY);
    BitBlt(hdc, 0, 0, cw, ch, p->mem_dc, 0, 0, SRCCOPY);
    ReleaseDC(p->hwnd, hdc);
    if (p->viewer != NULL)
        p->viewer->frames_drawn++;
}

/** Compose chrome when present, snapshot guest FB, then paint. */
void haeun_viewer_refresh_fb(viewer_plat *p) {
    uint32_t w = 0u, h = 0u;
    if (p == NULL || p->viewer == NULL || p->viewer->vm == NULL)
        return;
    if (p->viewer->chrome != NULL)
        (void)haeun_gfx_chrome_present(p->viewer->chrome, 0x00141C28u);
    if (haeun_fb_view_snapshot_bgra(p->viewer->vm, p->bgra, p->bgra_cap, &w,
                                    &h) != HAEUN_FB_VIEW_OK)
        return;
    p->fb_w = w;
    p->fb_h = h;
    haeun_viewer_paint(p);
}
