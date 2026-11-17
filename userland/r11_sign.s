# One sleep of 400 ticks. Host places COUNT near 0x80000000.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 8
    addi r2, r0, 400
    ecall
    addi r1, r0, 0
    addi r2, r0, 7
    ecall
    halt
