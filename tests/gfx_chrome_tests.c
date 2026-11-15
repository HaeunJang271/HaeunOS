#include "haeun/gfx.h"
#include "haeun/gfx_chrome.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum { FB = 0x1000u, A = 0x3000u, B = 0x4000u, C = 0x5000u };

static int test_desktop_lifecycle(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_chrome ch;
    int a = -1, b = -1, c = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, FB) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_init(&ch, &gfx, &wm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 8, 16u, 12u, A, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 12, 10, 16u, 12u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 20, 12, 16u, 12u, C, &c) == HAEUN_GFX_OK);
    CHECK(a != b && b != c);
    CHECK(haeun_gfx_wm_set_title(&wm, a, "A") == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, a, 0x000000FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, b, 0x0000FF00u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, c, 0x00FF0000u) == HAEUN_GFX_OK);

    /* Focus B via left title body (x=12); buttons occupy x>=13. */
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(&ch, 12, 5, 1u) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == b);
    CHECK(haeun_gfx_chrome_pointer(&ch, 12, 5, 0u) == HAEUN_GFX_OK);

    /* Drag B (+8,+3). */
    CHECK(haeun_gfx_chrome_pointer(&ch, 12, 5, 1u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(&ch, 20, 8, 1u) == HAEUN_GFX_OK);
    CHECK(wm.windows[b].x == 20);
    CHECK(wm.windows[b].y == 13);
    CHECK(haeun_gfx_chrome_pointer(&ch, 20, 8, 0u) == HAEUN_GFX_OK);

    /* Resize SE inward (display size <= immutable backing). */
    CHECK(haeun_gfx_chrome_pointer(&ch, 19, 19, 0u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(&ch, 19, 19, 1u) == HAEUN_GFX_OK);
    CHECK(ch.resize_id == a);
    CHECK(haeun_gfx_chrome_pointer(&ch, 15, 17, 1u) == HAEUN_GFX_OK);
    CHECK(wm.windows[a].w == 12u);
    CHECK(wm.windows[a].h == 10u);
    CHECK(wm.windows[a].w <= wm.windows[a].bw);
    CHECK(haeun_gfx_chrome_pointer(&ch, 15, 17, 0u) == HAEUN_GFX_OK);

    CHECK(haeun_gfx_wm_minimize(&wm, a) == HAEUN_GFX_OK);
    CHECK(wm.windows[a].minimized == 1);
    CHECK(wm.focus_id != a);
    CHECK(haeun_gfx_wm_restore(&wm, a) == HAEUN_GFX_OK);
    CHECK(wm.windows[a].minimized == 0);

    CHECK(haeun_gfx_wm_maximize(&wm, c, 64u, 32u) == HAEUN_GFX_OK);
    CHECK(wm.windows[c].maximized == 1);
    CHECK(wm.windows[c].x == 0 && wm.windows[c].y == 0);
    CHECK(haeun_gfx_wm_restore(&wm, c) == HAEUN_GFX_OK);
    CHECK(wm.windows[c].maximized == 0);

    CHECK(haeun_gfx_chrome_key(&ch, 0x1Cu) == HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_focus(&wm, b);
    CHECK(haeun_gfx_chrome_key(&ch, 0x1Cu) == HAEUN_GFX_OK);
    CHECK(ch.last_key == 0x1Cu);

    CHECK(haeun_gfx_chrome_present(&ch, 0x000000FFu) == HAEUN_GFX_OK);

    /* Close B via chrome button (raise first so C does not steal the hit). */
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(&ch, 0, 0, 0u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(
              &ch, wm.windows[b].x + (int32_t)wm.windows[b].w - 2,
              wm.windows[b].y - 4, 1u) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].used);
    CHECK(haeun_gfx_chrome_pointer(&ch, 0, 0, 0u) == HAEUN_GFX_OK);
    return 0;
}

int main(void) {
    if (test_desktop_lifecycle() != 0)
        return 1;
    puts("all gfx chrome tests passed");
    return 0;
}
