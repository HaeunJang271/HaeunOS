# Twenty yields, then EXIT 11. One runnable process.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 20
y_loop:
    addi r1, r0, 10
    ecall
    addi r8, r8, -1
    bne r8, r0, y_loop
    addi r1, r0, 0
    addi r2, r0, 11
    ecall
    halt
