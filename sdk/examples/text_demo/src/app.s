# text_demo — ASCII, digits, newline, multi-color WIN_DRAW_TEXT.
.isa 4
.global user_init

user_init:
    addi r2, r0, 2
    addi r3, r0, 2
    addi r4, r0, 56
    addi r5, r0, 28
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
    addi r3, r0, 0
    addi r4, r0, 0
    jal r5, fg1
    .word 0x00E0E0E0
fg1:
    lw r5, 0(r5)
    jal r6, bg1
    .word 0x00101020
bg1:
    lw r6, 0(r6)
    jal r7, s1
    .word 0x6C6C6548
    .word 0x0000006F
s1:
    jal r15, app_draw_text

    addi r2, r8, 0
    addi r3, r0, 0
    addi r4, r0, 8
    jal r5, fg2
    .word 0x00E0E000
fg2:
    lw r5, 0(r5)
    jal r6, bg1b
    .word 0x00101020
bg1b:
    lw r6, 0(r6)
    jal r7, s2
    # "1234" — avoid .word values whose high opcode nibble looks like JAL/BEQ
    .word 0x34333231
    .word 0x00000000
s2:
    jal r15, app_draw_text

    addi r2, r8, 0
    addi r3, r0, 0
    addi r4, r0, 16
    jal r5, fg3
    .word 0x0000E0E0
fg3:
    lw r5, 0(r5)
    jal r6, bg1c
    .word 0x00101020
bg1c:
    lw r6, 0(r6)
    jal r7, s3
    .word 0x00420A41
s3:
    jal r15, app_draw_text

    jal r15, app_compose
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf1
    jal r15, app_exit
