# M25.3 snapshot: stack word + code lw survive FORK into child.
# Markers: S=setup, C=child saw stack+code, P=parent after WAIT, Z=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    # Push pattern on ustack (writable private page).
    addi r13, r13, -4
    jal r1, p_tag
    .word 0x12340042
p_tag:
    lw r2, 0(r1)
    sw r2, 0(r13)
    # Sample first word of user code page (RX, readable).
    jal r1, p_code
    .word 0x00108000
p_code:
    lw r9, 0(r1)
    lw r10, 0(r9)
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x53
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    cmplt r3, r1, r0
    bne r3, r0, fail
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

child:
    lw r1, 0(r13)
    jal r2, p_exp
    .word 0x12340042
p_exp:
    lw r2, 0(r2)
    bne r1, r2, bad
    jal r1, p_c2
    .word 0x00108000
p_c2:
    lw r1, 0(r1)
    lw r1, 0(r1)
    bne r1, r10, bad
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x43
    ecall
    addi r1, r0, 0
    addi r2, r0, 7
    ecall
    halt

bad:
    addi r1, r0, 0
    addi r2, r0, 0xE1
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
