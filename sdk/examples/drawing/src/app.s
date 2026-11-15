# Drawing Demo — fill + diagonal pixels (no text / event APIs).
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    addi r2, r0, 4
    addi r3, r0, 4
    addi r4, r0, 20
    addi r5, r0, 16
    jal r6, base
    .word 0x00021000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    addi r2, r8, 0
    jal r3, bg
    .word 0x00202040
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    addi r9, r0, 0
    addi r12, r0, 16
pix:
    cmplt r1, r9, r12
    beq r1, r0, done_pix
    addi r2, r8, 0
    addi r3, r9, 0
    addi r4, r9, 0
    jal r5, col
    .word 0x00E0E000
col:
    lw r5, 0(r5)
    jal r15, app_win_pixel
    addi r9, r9, 1
    jal r0, pix
done_pix:
    addi r2, r8, 0
    jal r15, app_win_raise
    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    lw r2, -4(r13)
    jal r15, app_exit
bad:
    addi r2, r0, 0xd1
    jal r15, app_exit
