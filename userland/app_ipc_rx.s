# M20.2 ipc_receiver: create endpoint, register "ipcrx", recv one message.
.isa 4
.global user_init

user_init:
    jal r15, app_ipc_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    jal r2, nm
    .word 0x72637069
    .word 0x00000078
nm:
    addi r3, r8, 0
    jal r15, app_svc_reg
    cmplt r3, r1, r0
    bne r3, r0, bad

    addi r9, r0, 0
    addi r11, r0, 30000
wait:
    addi r2, r8, 0
    jal r15, app_ipc_avail
    cmplt r3, r1, r0
    bne r3, r0, bad
    bne r1, r0, got
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r11
    bne r3, r0, wait
    addi r2, r0, 0xee
    jal r15, app_exit
got:
    addi r2, r8, 0
    jal r15, app_ipc_peek
    beq r1, r0, bad
    cmplt r3, r0, r2
    beq r3, r0, bad
    addi r7, r2, 0
    addi r2, r8, 0
    addi r3, r13, -16
    addi r4, r0, 8
    jal r15, app_ipc_recv
    addi r4, r0, 4
    cmplt r3, r1, r4
    bne r3, r0, bad
    lw r3, -16(r13)
    jal r4, expect
    .word 0x00676D52
expect:
    lw r4, 0(r4)
    bne r3, r4, bad
    addi r2, r7, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xe2
    jal r15, app_exit
