# M24.2 lifecycle contract: FORK/YIELD/WAIT/EXIT UART markers.
# A=init, C=child ran, R=parent before WAIT, F=after reap, X=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x41
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x52
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x46
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x58
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    addi r1, r0, 10
    ecall
    addi r1, r0, 0
    addi r2, r0, 7
    ecall
    halt
