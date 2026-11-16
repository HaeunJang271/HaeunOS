# M25.4 FORK+guard: child overflows; parent stack intact; reuse FORK clean.
# Markers: A=start, C=child tagged, P=parent after wait, R=reuse child ok, Z.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r13, r13, -4
    jal r1, p_ok
    .word 0x51515252
p_ok:
    lw r2, 0(r1)
    sw r2, 0(r13)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x41
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_boom
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    # Parent stack word unchanged.
    lw r1, 0(r13)
    jal r2, p_chk
    .word 0x51515252
p_chk:
    lw r2, 0(r2)
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x50
    ecall
    # Reuse slot: FORK again; child must not see boom tag.
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

child_boom:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    # Distinct tag then hit guard.
    jal r1, p_bad
    .word 0x44434241
p_bad:
    lw r2, 0(r1)
    sw r2, 0(r13)
    addi r1, r14, -4
    lw r1, 0(r1)
    halt

child_reuse:
    lw r1, 0(r13)
    jal r2, p_exp
    .word 0x51515252
p_exp:
    lw r2, 0(r2)
    bne r1, r2, bad
    jal r2, p_no
    .word 0x44434241
p_no:
    lw r2, 0(r2)
    beq r1, r2, bad
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x52
    ecall
    addi r1, r0, 0
    addi r2, r0, 3
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
