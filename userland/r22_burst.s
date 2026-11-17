# Bursty: alternate short spin and YIELD, then EXIT 26.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 20
burst:
    addi r9, r0, 30
spin:
    addi r9, r9, -1
    bne r9, r0, spin
    addi r1, r0, 10
    ecall
    addi r8, r8, -1
    bne r8, r0, burst
    addi r1, r0, 0
    addi r2, r0, 26
    ecall
    halt
