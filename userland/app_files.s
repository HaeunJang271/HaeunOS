# Files: lists app registry names; stays open until Esc/CLOSE.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    addi r1, r0, 18
    addi r2, r0, 112
    addi r3, r0, 56
    addi r4, r0, 160
    addi r5, r0, 80
    jal r6, base
    .word 0x00080000
base:
    lw r6, 0(r6)
    ecall
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad
    sw r8, -8(r13)

    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, bg
    .word 0x00182028
bg:
    lw r3, 0(r3)
    ecall

    addi r1, r0, 28
    addi r2, r8, 0
    addi r3, r0, 8
    addi r4, r0, 8
    jal r5, fg
    .word 0x00E8F0FF
fg:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, t_fl
    .word 0x656C6946
    .word 0x00000073
t_fl:
    ecall

    addi r1, r0, 24
    ecall
    sw r1, -12(r13)
    beq r1, r0, shown
    # draw first app name
    addi r1, r0, 25
    addi r2, r0, 0
    addi r3, r13, -48
    # Cap must be < HAEUN_APP_NAME_MAX (16); 16 is rejected by host.
    addi r4, r0, 15
    ecall
    cmplt r3, r1, r0
    bne r3, r0, shown
    addi r1, r0, 28
    lw r2, -8(r13)
    addi r3, r0, 8
    addi r4, r0, 28
    jal r5, fg2
    .word 0x00A0E0FF
fg2:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -48
    ecall

shown:
    addi r1, r0, 23
    addi r2, r0, 0
    ecall

loop:
    addi r1, r0, 27
    ecall
    beq r1, r0, idle
    addi r10, r0, 8
    beq r1, r10, bye
    addi r10, r0, 1
    bne r1, r10, loop
    addi r10, r0, 0x01
    beq r3, r10, bye
    jal r0, loop
idle:
    addi r1, r0, 37
    lw r2, -8(r13)
    ecall
    bne r1, r0, bye
    addi r1, r0, 10
    ecall
    jal r0, loop
bye:
    addi r1, r0, 37
    lw r2, -8(r13)
    ecall
    bne r1, r0, bye_exit
    addi r1, r0, 19
    lw r2, -8(r13)
    ecall
bye_exit:
    lw r2, -4(r13)
    addi r1, r0, 0
    ecall
    halt
bad:
    addi r1, r0, 0
    addi r2, r0, 0xc3
    ecall
    halt
