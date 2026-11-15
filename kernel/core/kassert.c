/* Kernel assertions. */
extern int kpanic(int code);

int kassert(int cond) {
    int st = 0;
    if (cond == 0) {
        st = kpanic(5);
    }
    return st;
}
