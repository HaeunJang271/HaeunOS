# M20 file manager lite: create/read note + draw status text.
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    addi r2, r0, 8
    addi r3, r0, 6
    addi r4, r0, 40
    addi r5, r0, 18
    jal r6, base
    .word 0x00022000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0

    jal r2, path
    .word 0x65746f6e
    .word 0x7478742e
    .word 0x00000000
path:
    addi r3, r0, 1
    jal r15, app_open
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r9, r1, 0
    addi r2, r9, 0
    addi r3, r0, 79
    jal r15, app_write
    addi r2, r9, 0
    addi r3, r0, 75
    jal r15, app_write
    addi r2, r9, 0
    jal r15, app_close

    addi r2, r8, 0
    jal r3, bg
    .word 0x00202820
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r0, 2
    jal r5, fg
    .word 0x00F0F0F0
fg:
    lw r5, 0(r5)
    jal r6, bg2
    .word 0x00202820
bg2:
    lw r6, 0(r6)
    jal r7, label
    .word 0x454c4946
    .word 0x00000000
label:
    jal r15, app_draw_text
    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    lw r2, -4(r13)
    jal r15, app_exit
fail:
    addi r2, r0, 0xe2
    jal r15, app_exit
