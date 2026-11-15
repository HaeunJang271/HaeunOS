# M20.1: verify empty queue at start; create window; ensure foreign keys absent.
# Isolation is proven by host unit tests; this app exits 0 if start empty.
.isa 4
.global user_init

user_init:
    jal r15, app_event_poll
    bne r1, r0, leak

    addi r2, r0, 4
    addi r3, r0, 4
    addi r4, r0, 20
    addi r5, r0, 12
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    cmplt r3, r1, r0
    bne r3, r0, bad

    # Still should not see another pid's events without host inject.
    jal r15, app_event_poll
    # FOCUS (6) may arrive from auto-focus — allow FOCUS only
    beq r1, r0, ok
    addi r4, r0, 6
    beq r1, r4, ok
    addi r2, r0, 0xef
    jal r15, app_exit
ok:
    addi r2, r0, 0
    jal r15, app_exit
leak:
    addi r2, r0, 0xed
    jal r15, app_exit
bad:
    addi r2, r0, 0xe0
    jal r15, app_exit
