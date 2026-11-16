# M25.6 lifecycle: child heap tag then EXIT; reuse FORK gets parent snapshot. L R Z
.isa 4
.global user_init
.global heap_init
.global malloc

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    jal r15, heap_init
    addi r1, r0, 16
    jal r15, malloc
    beq r1, r0, fail
    addi r9, r1, 0
    jal r1, p_keep
    .word 0x51510005
p_keep:
    lw r1, 0(r1)
    sw r1, 0(r9)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4C
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_die
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_reuse
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child_die:
    jal r1, p_bad
    .word 0x44440004
p_bad:
    lw r1, 0(r1)
    sw r1, 0(r9)
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

child_reuse:
    lw r1, 0(r9)
    jal r2, p_exp
    .word 0x51510005
p_exp:
    lw r2, 0(r2)
    bne r1, r2, bad
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x52
    ecall
    addi r1, r0, 0
    addi r2, r0, 2
    ecall
    halt

bad:
    addi r1, r0, 0
    addi r2, r0, 0xE4
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
