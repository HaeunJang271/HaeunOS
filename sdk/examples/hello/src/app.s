# Hello World — UART bytes via app_putc, then EXIT.
.isa 4
.global user_init

user_init:
    addi r2, r0, 72
    jal r15, app_putc
    addi r2, r0, 105
    jal r15, app_putc
    addi r2, r0, 10
    jal r15, app_putc
    addi r2, r0, 0
    jal r15, app_exit
