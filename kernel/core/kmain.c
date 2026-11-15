/* Foundation init sequence. Required steps must succeed before ready log. */
extern int cpu_init(void);
extern int trap_init(void);
extern int uart_init(void);
extern int klog_init(void);
extern int klog_banner(void);
extern int klog_ready(void);
extern int kpanic(int code);
extern int kassert(int cond);
extern int pmm_init(void);
extern int vmm_init(void);
extern int heap_init(void);
extern int pmm_alloc(void);
extern int pmm_free(int page);
extern int heap_alloc(int size);

int kmain(void) {
    int st = 0;
    int page = 0;
    int block = 0;
    int failed = 0;
    st = cpu_init();
    if (st != 0) {
        failed = 1;
    }
    if (failed == 0) {
        st = trap_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = uart_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = klog_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = klog_banner();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = pmm_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = vmm_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = heap_init();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        page = pmm_alloc();
        st = kassert(page != 0);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = pmm_free(page);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        block = heap_alloc(16);
        st = kassert(block != 0);
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed == 0) {
        st = klog_ready();
        if (st != 0) {
            failed = 1;
        }
    }
    if (failed != 0) {
        st = kpanic(st);
    }
    return st;
}
