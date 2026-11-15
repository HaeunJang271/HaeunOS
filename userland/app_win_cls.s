# M20.4 window_close_demo: destroy after create; ensure DESTROYED != CLOSE.
# Soft CLOSE_REQUEST is host-chrome; here app destroys and expects type 17.
.isa 4
.global user_init

user_init:
    addi r2, r0, 6
    addi r3, r0, 6
    addi r4, r0, 20
    addi r5, r0, 12
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    addi r9, r0, 0
    addi r12, r0, 48
drain:
    jal r15, app_event_poll
    bne r1, r0, drain_more
    jal r0, after_drain
drain_more:
    addi r4, r0, 8
    beq r1, r4, bad
    jal r0, drain
after_drain:
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, drain

    addi r2, r8, 0
    jal r15, app_win_destroy

    addi r9, r0, 0
    addi r12, r0, 64
    addi r10, r0, 0
wait:
    jal r15, app_event_poll
    addi r4, r0, 17
    beq r1, r4, got_d
    addi r4, r0, 8
    beq r1, r4, bad
    jal r0, spin
got_d:
    addi r10, r0, 1
spin:
    addi r3, r10, 0
    bne r3, r0, ok
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, wait
    jal r0, bad
ok:
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf6
    jal r15, app_exit
