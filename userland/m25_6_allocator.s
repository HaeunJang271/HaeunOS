# M25.6 allocator API smoke (reorder avoids live calloc during realloc).
.isa 4
.global user_init
.global heap_init
.global malloc
.global free
.global calloc
.global realloc

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    jal r15, heap_init
    addi r1, r0, 32
    jal r15, malloc
    beq r1, r0, fail
    addi r9, r1, 0
    addi r1, r0, 0x41
    sw r1, 0(r9)
    addi r1, r9, 0
    addi r2, r0, 48
    jal r15, realloc
    beq r1, r0, fail
    lw r2, 0(r1)
    addi r3, r0, 0x41
    bne r2, r3, fail
    jal r15, free
    addi r1, r0, 3
    addi r2, r0, 4
    jal r15, calloc
    beq r1, r0, fail
    lw r2, 0(r1)
    bne r2, r0, fail
    jal r15, free
    addi r1, r0, 5000
    jal r15, malloc
    bne r1, r0, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x41
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4F
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
