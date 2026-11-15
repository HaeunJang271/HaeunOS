# M20.4 window_resize_demo: create, query size, destroy → DESTROYED polled.
.isa 4
.global user_init

user_init:
    addi r2, r0, 2
    addi r3, r0, 2
    addi r4, r0, 24
    addi r5, r0, 16
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    # drain CREATED/FOCUS
    addi r9, r0, 0
    addi r12, r0, 32
drain:
    jal r15, app_event_poll
    bne r1, r0, drain
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, drain

    addi r2, r8, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    # r4 low 16 = w should be 24
    jal r1, wmask
    .word 0x0000ffff
wmask:
    lw r1, 0(r1)
    and r1, r4, r1
    addi r3, r0, 24
    bne r1, r3, bad

    addi r2, r8, 0
    jal r15, app_win_destroy

    addi r9, r0, 0
    addi r12, r0, 64
wait_d:
    jal r15, app_event_poll
    addi r4, r0, 17
    beq r1, r4, ok
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, wait_d
    jal r0, bad
ok:
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf5
    jal r15, app_exit
