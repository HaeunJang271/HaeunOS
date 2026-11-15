# UART MMIO driver (DATA at 0x10002000).
.isa 4
.global uart_init
.global uart_putc

uart_init:
    jal r1, p_base
    .word 0x10002000
p_base:
    lw r1, 0(r1)
    sw r0, 8(r1)
    addi r1, r0, 0
    jalr r0, r15, 0

# uart_putc(c): write low byte of r2 to UART DATA; return 0.
uart_putc:
    jal r1, p_uart
    .word 0x10002000
p_uart:
    lw r1, 0(r1)
    sw r2, 0(r1)
    addi r1, r0, 0
    jalr r0, r15, 0
