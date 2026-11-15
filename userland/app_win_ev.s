# M20.4 window_event_viewer: expect CREATED then FOCUS; show via query.
.isa 4
.global user_init

user_init:
    addi r2, r0, 4
    addi r3, r0, 4
    addi r4, r0, 32
    addi r5, r0, 16
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    addi r10, r0, 0
    addi r11, r0, 0
    addi r9, r0, 0
    addi r12, r0, 128
poll:
    jal r15, app_event_poll
    beq r1, r0, spin
    addi r4, r0, 11
    beq r1, r4, mark_c
    addi r4, r0, 6
    beq r1, r4, mark_f
    jal r0, spin
mark_c:
    addi r10, r0, 1
    jal r0, spin
mark_f:
    addi r11, r0, 1
spin:
    addi r3, r10, 0
    beq r3, r0, cont
    addi r3, r11, 0
    bne r3, r0, ok
cont:
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, poll
    jal r0, bad
ok:
    addi r2, r8, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf4
    jal r15, app_exit
