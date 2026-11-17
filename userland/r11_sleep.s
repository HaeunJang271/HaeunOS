# Three short sleeps, then EXIT 33.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 3
s_loop:
    addi r1, r0, 8
    addi r2, r0, 16
    ecall
    addi r8, r8, -1
    bne r8, r0, s_loop
    addi r1, r0, 0
    addi r2, r0, 33
    ecall
    halt
