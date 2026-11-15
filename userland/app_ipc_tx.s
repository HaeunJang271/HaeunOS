# M20.2 ipc_sender: lookup "ipcrx", send payload, confirm success.
.isa 4
.global user_init

user_init:
    addi r9, r0, 0
    addi r11, r0, 30000
find:
    jal r2, nm
    .word 0x72637069
    .word 0x00000078
nm:
    jal r15, app_svc_lookup
    cmplt r3, r1, r0
    beq r3, r0, found
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r11
    bne r3, r0, find
    addi r2, r0, 0xef
    jal r15, app_exit
found:
    addi r8, r1, 0
    jal r3, payload
    .word 0x00676D52
payload:
    addi r2, r8, 0
    addi r4, r0, 4
    jal r15, app_ipc_send
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xe3
    jal r15, app_exit
