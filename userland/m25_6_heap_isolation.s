# M25.6 FORK heap: child sees snapshot; parent mutate isolated. I C P Z
.isa 4
.global user_init
.global heap_init
.global malloc
.global free

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
    jal r1, p_v1
    .word 0x11110001
p_v1:
    lw r1, 0(r1)
    sw r1, 0(r9)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x49
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    cmplt r3, r1, r0
    bne r3, r0, fail
    jal r1, p_v2
    .word 0x22220002
p_v2:
    lw r1, 0(r1)
    sw r1, 0(r9)
    addi r1, r0, 7
    ecall
    lw r1, 0(r9)
    jal r2, p_v3
    .word 0x22220002
p_v3:
    lw r2, 0(r2)
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x50
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child:
    lw r1, 0(r9)
    jal r2, p_c1
    .word 0x11110001
p_c1:
    lw r2, 0(r2)
    bne r1, r2, bad
    jal r1, p_c2
    .word 0x33330003
p_c2:
    lw r1, 0(r1)
    sw r1, 0(r9)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

bad:
    addi r1, r0, 0
    addi r2, r0, 0xE2
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
