# File Viewer — create/read note.txt via HFS OPEN assist, paint result.
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    addi r2, r0, 12
    addi r3, r0, 6
    addi r4, r0, 24
    addi r5, r0, 12
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
    addi r3, r0, 72
    jal r15, app_write
    addi r2, r9, 0
    addi r3, r0, 73
    jal r15, app_write
    addi r2, r9, 0
    jal r15, app_close

    jal r2, path2
    .word 0x65746f6e
    .word 0x7478742e
    .word 0x00000000
path2:
    addi r3, r0, 0
    jal r15, app_open
    addi r9, r1, 0
    addi r2, r9, 0
    jal r15, app_read
    addi r4, r0, 72
    bne r1, r4, fail
    addi r2, r9, 0
    jal r15, app_close

    addi r2, r8, 0
    jal r3, ok
    .word 0x00004080
ok:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    lw r2, -4(r13)
    jal r15, app_exit
fail:
    addi r2, r8, 0
    jal r3, er
    .word 0x000000A0
er:
    lw r3, 0(r3)
    jal r15, app_win_fill
    addi r2, r0, 0xc2
    jal r15, app_exit
