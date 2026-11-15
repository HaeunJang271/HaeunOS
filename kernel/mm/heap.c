/* Bump heap backed by PMM pages. */
extern int mem_load(int addr);
extern int mem_store(int addr, int value);
extern int pmm_alloc(void);
extern int pmm_meta(void);

int heap_init(void) {
    int meta = 0;
    int st = 0;
    meta = pmm_meta();
    st = mem_store(meta + 8, 0);
    if (st == 0) {
        st = mem_store(meta + 12, 0);
    }
    return st;
}

int heap_alloc(int size) {
    int meta = 0;
    int cur = 0;
    int end = 0;
    int page = 0;
    int need = 0;
    int st = 0;
    int result = 0;
    int failed = 0;
    if (size <= 0) {
        failed = 1;
    }
    if (failed == 0) {
        need = size + 3;
        need = need - (need & 3);
        meta = pmm_meta();
        cur = mem_load(meta + 8);
        end = mem_load(meta + 12);
        if (cur == 0) {
            page = pmm_alloc();
            if (page == 0) {
                failed = 1;
            }
            if (failed == 0) {
                cur = page;
                end = page + 4096;
                st = mem_store(meta + 8, cur);
                if (st != 0) {
                    failed = 1;
                }
            }
            if (failed == 0) {
                st = mem_store(meta + 12, end);
                if (st != 0) {
                    failed = 1;
                }
            }
        }
    }
    if (failed == 0) {
        if (cur + need > end) {
            page = pmm_alloc();
            if (page == 0) {
                failed = 1;
            }
            if (failed == 0) {
                cur = page;
                end = page + 4096;
                st = mem_store(meta + 12, end);
                if (st != 0) {
                    failed = 1;
                }
            }
        }
    }
    if (failed == 0) {
        st = mem_store(meta + 8, cur + need);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        result = cur;
    }
    return result;
}
