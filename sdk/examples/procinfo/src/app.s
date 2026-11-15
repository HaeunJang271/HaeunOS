# Process Information — GETPID + APP_COUNT, paint by pid bit.
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)
    addi r10, r1, 0

    jal r15, app_count
    addi r11, r1, 0
    beq r11, r0, bad

    addi r2, r0, 10
    addi r3, r0, 8
    addi r4, r0, 28
    addi r5, r0, 12
    jal r6, base
    .word 0x00023000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    # Color from low bit of pid (visual only; no text).
    jal r1, one
    .word 1
one:
    lw r1, 0(r1)
    and r1, r10, r1
    beq r1, r0, even
    addi r2, r8, 0
    jal r3, oddc
    .word 0x00800040
oddc:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r0, finish
even:
    addi r2, r8, 0
    jal r3, evenc
    .word 0x00408000
evenc:
    lw r3, 0(r3)
    jal r15, app_win_fill
finish:
    jal r15, app_compose
    addi r2, r8, 0
    jal r15, app_win_destroy
    jal r15, app_yield
    lw r2, -4(r13)
    jal r15, app_exit
bad:
    addi r2, r0, 0xd2
    jal r15, app_exit
