# M20.2 pingpong server: words [type=1][seq][reply_epid] → PONG type=2.
.isa 4
.global user_init

user_init:
    jal r15, app_ipc_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad
    jal r2, nm
    .word 0x676E6F70
    .word 0x00000000
nm:
    addi r3, r8, 0
    jal r15, app_svc_reg
    cmplt r3, r1, r0
    bne r3, r0, bad

    addi r9, r0, 0
    addi r11, r0, 40000
wait:
    addi r2, r8, 0
    addi r3, r13, -16
    addi r4, r0, 12
    jal r15, app_ipc_recv
    bne r1, r0, got
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r11
    bne r3, r0, wait
    addi r2, r0, 0xee
    jal r15, app_exit
got:
    lw r3, -16(r13)
    addi r4, r0, 1
    bne r3, r4, bad
    lw r5, -12(r13)
    lw r6, -8(r13)
    addi r3, r0, 2
    sw r3, -16(r13)
    sw r5, -12(r13)
    addi r2, r6, 0
    addi r3, r13, -16
    addi r4, r0, 8
    jal r15, app_ipc_send
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xe4
    jal r15, app_exit
