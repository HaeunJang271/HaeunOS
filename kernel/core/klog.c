/* Kernel logging control path. */
extern int uart_putc(int ch);

int klog_init(void) {
    return 0;
}

int klog_putc(int ch) {
    int st = 0;
    st = uart_putc(ch);
    return st;
}
