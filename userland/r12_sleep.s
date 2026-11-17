# SLEEP then EXIT 23.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 8
    addi r2, r0, 64
    ecall
    addi r1, r0, 0
    addi r2, r0, 23
    ecall
    halt
