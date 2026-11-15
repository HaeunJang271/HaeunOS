/* Kernel panic: log code then halt via helper. */
extern int klog_panic(int code);
extern int panic_halt(int code);

int kpanic(int code) {
    int st = 0;
    st = klog_panic(code);
    st = panic_halt(code);
    return st;
}
