# Spin in user mode so timer IRQs are taken from user, not from ERET.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 1
spin:
    addi r8, r8, 1
    jal r0, spin
