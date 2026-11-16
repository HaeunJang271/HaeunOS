# M23.2: exits only on EVT_CLOSE (type 8); Esc must not terminate.
.isa 4
.global user_init

user_init:
    addi r2, r0, 72
    addi r3, r0, 36
    addi r4, r0, 176
    addi r5, r0, 88
    jal r6, wbase
    .word 0x00060000
wbase:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad
    addi r2, r8, 0
    jal r3, fillc
    .word 0x004080ff
fillc:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r2, desk
    .word 0x203040ff
desk:
    lw r2, 0(r2)
    jal r15, app_compose

    addi r9, r0, 0
    addi r12, r0, 400
poll:
    jal r15, app_event_poll
    beq r1, r0, spin
    addi r4, r0, 8
    beq r1, r4, bye
    # Ignore KEY and other events (including Esc).
spin:
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, poll
    jal r0, bad
bye:
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf8
    jal r15, app_exit
