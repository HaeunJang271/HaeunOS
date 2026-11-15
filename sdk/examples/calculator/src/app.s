# Calculator — compute 12+34, paint window green on success (no text API).
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    addi r2, r0, 8
    addi r3, r0, 4
    addi r4, r0, 24
    addi r5, r0, 12
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
    .word 0x00101010
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    addi r4, r0, 12
    addi r5, r0, 34
    add r6, r4, r5
    addi r7, r0, 46
    bne r6, r7, bad_math

    addi r2, r8, 0
    jal r3, okc
    .word 0x0000C000
okc:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    lw r2, -4(r13)
    jal r15, app_exit

bad_math:
    addi r2, r8, 0
    jal r3, erc
    .word 0x000000C0
erc:
    lw r3, 0(r3)
    jal r15, app_win_fill
bad:
    addi r2, r0, 0xc1
    jal r15, app_exit
