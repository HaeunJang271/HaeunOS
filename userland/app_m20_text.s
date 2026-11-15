# M20 text demo: window + WIN_DRAW_TEXT + EXIT.
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    addi r2, r0, 2
    addi r3, r0, 8
    addi r4, r0, 40
    addi r5, r0, 16
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    addi r2, r8, 0
    jal r3, bg
    .word 0x00101020
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r0, 2
    jal r5, fg
    .word 0x00E0E0E0
fg:
    lw r5, 0(r5)
    jal r6, bg2
    .word 0x00101020
bg2:
    lw r6, 0(r6)
    jal r7, msg
    .word 0x4c4c4548
    .word 0x0000004f
msg:
    jal r15, app_draw_text
    cmplt r3, r1, r0
    bne r3, r0, bad

    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    lw r2, -4(r13)
    jal r15, app_exit
bad:
    addi r2, r0, 0xe0
    jal r15, app_exit
