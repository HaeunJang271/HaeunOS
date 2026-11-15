#include "haeun/fb_view.h"
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

/** RGBA→BGRA snapshot and mouse mapping. */
static int test_snapshot_and_map(void) {
    uint8_t ram[256];
    uint8_t out[64];
    haeun_vm vm;
    uint32_t w = 0u, h = 0u;
    int32_t fx = 0, fy = 0;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    vm.fb_width = 2u;
    vm.fb_height = 2u;
    vm.fb_base = 0u;
    vm.fb_control = 1u;
    /* pixel0 RGBA = R=1 G=2 B=3 A=4 */
    ram[0] = 1;
    ram[1] = 2;
    ram[2] = 3;
    ram[3] = 4;
    CHECK(haeun_fb_view_snapshot_bgra(&vm, out, sizeof(out), &w, &h) ==
          HAEUN_FB_VIEW_OK);
    CHECK(w == 2u && h == 2u);
    CHECK(out[0] == 3 && out[1] == 2 && out[2] == 1 && out[3] == 4);
    CHECK(haeun_fb_view_map_mouse(8, 8, 64, 32, 8u, 4u, &fx, &fy) == 0);
    CHECK(fx == 1 && fy == 1);
    CHECK(haeun_fb_view_map_mouse(1000, 1000, 64, 32, 8u, 4u, &fx, &fy) == -1);
    return 0;
}

int main(void) {
    if (test_snapshot_and_map() != 0)
        return 1;
    puts("all fb view tests passed");
    return 0;
}
