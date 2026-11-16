# M24.3 C: repeated parent-exit-before-child; init WAITs both each cycle.
# Marker R per cycle, T=done. Loops 4 times (AS budget).
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r4, r0, 4
cycle:
    addi r1, r0, 5
    ecall
    beq r1, r0, middle
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x52
    ecall
    addi r4, r4, -1
    bne r4, r0, cycle
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x54
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

middle:
    addi r1, r0, 5
    ecall
    beq r1, r0, kid
    addi r1, r0, 0
    addi r2, r0, 3
    ecall
    halt

kid:
    addi r1, r0, 0
    addi r2, r0, 9
    ecall
    halt
