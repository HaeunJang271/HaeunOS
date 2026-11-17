# Spin then EXIT 21. Runtime accounting smoke.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 200
spin:
    addi r8, r8, -1
    bne r8, r0, spin
    addi r1, r0, 0
    addi r2, r0, 21
    ecall
    halt
