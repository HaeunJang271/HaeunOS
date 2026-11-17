# WAIT with no child. Kernel must halt without resuming this PCB.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 7
    ecall
    addi r1, r0, 0
    addi r2, r0, 99
    ecall
    halt
