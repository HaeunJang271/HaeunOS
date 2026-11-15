# M20.1: poll KEY_DOWN via EVENT_POLL; exit with scancode in r2.
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
    jal r3, fillc
    .word 0x00204020
fillc:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r15, app_compose

    addi r9, r0, 0
    addi r11, r0, 20000
poll:
    jal r15, app_event_poll
    beq r1, r0, spin
    # KEY_DOWN == 1
    addi r4, r0, 1
    bne r1, r4, spin
    # r3 = scancode (a); exit with it
    addi r2, r3, 0
    jal r15, app_exit
spin:
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r11
    bne r3, r0, poll
    addi r2, r0, 0xee
    jal r15, app_exit
bad:
    addi r2, r0, 0xe0
    jal r15, app_exit
