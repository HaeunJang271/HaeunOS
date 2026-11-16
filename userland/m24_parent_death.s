# M24.2 parent death: middle EXITs with ZOMBIE grandchild; init WAITs both.
# M=middle, G=grandchild, P/Q=init reaps, Z=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 5
    ecall
    beq r1, r0, middle
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x50
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x51
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

middle:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4D
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, grandchild
    addi r1, r0, 0
    addi r2, r0, 3
    ecall
    halt

grandchild:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x47
    ecall
    addi r1, r0, 0
    addi r2, r0, 9
    ecall
    halt
