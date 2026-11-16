# M23.2: fill window table, recover after destroy, stress create/destroy.
.isa 4
.global user_init

user_init:
    addi r8, r0, 0
    jal r6, pool0
    .word 0x00020000
pool0:
    lw r11, 0(r6)
    addi r14, r13, -4

mk_loop:
    addi r2, r0, 8
    add r2, r2, r8
    addi r3, r0, 40
    addi r4, r0, 8
    addi r5, r0, 8
    addi r6, r11, 0
    jal r15, app_win_create
    cmplt r3, r1, r0
    bne r3, r0, full
    sw r1, 0(r14)
    addi r14, r14, -4
    addi r2, r1, 0
    jal r3, col
    .word 0x0000ff00
col:
    lw r3, 0(r3)
    jal r15, app_win_fill
    addi r11, r11, 0x400
    addi r8, r8, 1
    addi r3, r0, 8
    cmplt r4, r8, r3
    bne r4, r0, mk_loop
    # Filled 8 guest slots (no prior hosts) — still run stress
    jal r0, stress

full:
    # Capacity exhausted after creating at least one window
    cmplt r3, r0, r8
    beq r3, r0, bad

stress:
    # Destroy the first created window; recreate into freed capacity
    lw r2, -4(r13)
    jal r15, app_win_destroy
    addi r2, r0, 16
    addi r3, r0, 40
    addi r4, r0, 8
    addi r5, r0, 8
    jal r6, reb
    .word 0x00020000
reb:
    lw r6, 0(r6)
    jal r15, app_win_create
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r10, r1, 0

    addi r9, r0, 0
    addi r12, r0, 16
cyc:
    addi r2, r10, 0
    jal r15, app_win_destroy
    addi r2, r0, 16
    addi r3, r0, 40
    addi r4, r0, 8
    addi r5, r0, 8
    jal r6, reb2
    .word 0x00020000
reb2:
    lw r6, 0(r6)
    jal r15, app_win_create
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r10, r1, 0
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, cyc

    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf9
    jal r15, app_exit
