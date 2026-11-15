# M20.1: wait for MOUSE_DOWN; exit 0 with lx in r2 (via exit code path: r2).
.isa 4
.global user_init

user_init:
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
    .word 0x00202040
fillc:
    lw r3, 0(r3)
    jal r15, app_win_fill
    jal r15, app_compose

    addi r9, r0, 0
    addi r11, r0, 20000
poll:
    jal r15, app_event_poll
    beq r1, r0, spin
    # MOUSE_DOWN == 4; also accept MOVE(3)/UP(5) then keep going for DOWN
    addi r4, r0, 4
    bne r1, r4, maybe_move
    # success: pack lx (r3) as exit status
    addi r2, r3, 0
    jal r15, app_exit
maybe_move:
    addi r4, r0, 3
    beq r1, r4, spin
    addi r4, r0, 5
    beq r1, r4, spin
    # FOCUS/BLUR etc — ignore
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
