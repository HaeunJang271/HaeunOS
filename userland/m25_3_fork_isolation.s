# M25.3 isolation: parent/child stack writes must not cross AS.
# Markers: I=init, C=child wrote+checked parent pattern, P=parent ok, Z=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r13, r13, -4
    jal r1, p_par
    .word 0x11111111
p_par:
    lw r2, 0(r1)
    sw r2, 0(r13)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x49
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    cmplt r3, r1, r0
    bne r3, r0, fail
    # Parent overwrites own stack word after FORK.
    jal r1, p_p2
    .word 0x22222222
p_p2:
    lw r2, 0(r1)
    sw r2, 0(r13)
    addi r1, r0, 10
    ecall
    addi r1, r0, 7
    ecall
    # Parent still sees its own post-FORK value.
    lw r1, 0(r13)
    jal r2, p_p3
    .word 0x22222222
p_p3:
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
    # Must still see pre-FORK parent snapshot, not parent's later write.
    lw r1, 0(r13)
    jal r2, p_c1
    .word 0x11111111
p_c1:
    lw r2, 0(r2)
    bne r1, r2, bad
    jal r1, p_c2
    .word 0x33333333
p_c2:
    lw r2, 0(r1)
    sw r2, 0(r13)
    addi r1, r0, 10
    ecall
    lw r1, 0(r13)
    jal r2, p_c3
    .word 0x33333333
p_c3:
    lw r2, 0(r2)
    bne r1, r2, bad
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    addi r1, r0, 0
    addi r2, r0, 9
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
