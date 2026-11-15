/* Physical page freelist; metadata words live at fixed PAs via mem_*. */
extern int mem_load(int addr);
extern int mem_store(int addr, int value);
extern int pmm_base(void);
extern int pmm_end(void);
extern int pmm_meta(void);

int pmm_init(void) {
    int base = 0;
    int end = 0;
    int page = 0;
    int next = 0;
    int meta = 0;
    int st = 0;
    int failed = 0;
    base = pmm_base();
    end = pmm_end();
    meta = pmm_meta();
    if (base == 0) {
        failed = 1;
        st = 1;
    }
    if (failed == 0) {
        if (end <= base) {
            failed = 1;
            st = 1;
        }
    }
    if (failed == 0) {
        st = mem_store(meta, 0);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        page = end;
        while (page > base) {
            page = page - 4096;
            next = mem_load(meta);
            st = mem_store(page, next);
            if (st != 0) {
                failed = 1;
                page = base;
            }
            if (failed == 0) {
                st = mem_store(meta, page);
                if (st != 0) {
                    failed = 1;
                    page = base;
                }
            }
        }
    }
    if (failed == 0) {
        st = mem_store(meta + 4, 1);
    }
    return st;
}

int pmm_alloc(void) {
    int meta = 0;
    int page = 0;
    int next = 0;
    int ready = 0;
    int st = 0;
    meta = pmm_meta();
    ready = mem_load(meta + 4);
    if (ready == 0) {
        page = 0;
    }
    if (ready != 0) {
        page = mem_load(meta);
        if (page != 0) {
            next = mem_load(page);
            st = mem_store(meta, next);
        }
    }
    return page;
}

int pmm_free(int page) {
    int meta = 0;
    int base = 0;
    int end = 0;
    int ready = 0;
    int next = 0;
    int st = 0;
    int failed = 0;
    meta = pmm_meta();
    ready = mem_load(meta + 4);
    if (ready == 0) {
        failed = 1;
        st = 2;
    }
    if (failed == 0) {
        if (page == 0) {
            failed = 1;
            st = 1;
        }
    }
    if (failed == 0) {
        base = pmm_base();
        end = pmm_end();
        if (page < base) {
            failed = 1;
            st = 1;
        }
    }
    if (failed == 0) {
        if (page >= end) {
            failed = 1;
            st = 1;
        }
    }
    if (failed == 0) {
        next = mem_load(meta);
        st = mem_store(page, next);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = mem_store(meta, page);
    }
    return st;
}
