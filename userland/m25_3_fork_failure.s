# M25.3 failure: third FORK returns -1; parent state preserved (marker F/P/Z).
# Also: child exits; parent continues (lifecycle). Markers: A,B,C,F,P,Z
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
    # FORK #1 → child_a
    addi r1, r0, 5
    ecall
    beq r1, r0, child_a
    cmplt r3, r1, r0
    bne r3, r0, fail
    # FORK #2 → child_b
    addi r1, r0, 5
    ecall
    beq r1, r0, child_b
    cmplt r3, r1, r0
    bne r3, r0, fail
    # FORK #3 must fail (AS×3 exhausted: slots 0,1,2 busy)
    addi r1, r0, 5
    ecall
    cmplt r3, r1, r0
    beq r3, r0, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x46
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
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

child_a:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x42
    ecall
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

child_b:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    addi r1, r0, 0
    addi r2, r0, 2
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
