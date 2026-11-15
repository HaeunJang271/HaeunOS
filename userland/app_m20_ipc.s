# M20 IPC self-test: create endpoint, send to self via second create? 
# Instead: create, register service, send from same process (allowed), recv.
.isa 4
.global user_init

user_init:
    jal r15, app_ipc_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    jal r2, sname
    .word 0x6f686365
    .word 0x00000000
sname:
    addi r3, r8, 0
    jal r15, app_svc_reg
    cmplt r3, r1, r0
    bne r3, r0, bad

    jal r2, sname2
    .word 0x6f686365
    .word 0x00000000
sname2:
    jal r15, app_svc_lookup
    bne r1, r8, bad

    jal r3, payload
    .word 0x00004142
payload:
    addi r2, r8, 0
    addi r4, r0, 2
    jal r15, app_ipc_send
    cmplt r3, r1, r0
    bne r3, r0, bad

    addi r2, r8, 0
    addi r3, r13, -16
    addi r4, r0, 8
    jal r15, app_ipc_recv
    addi r4, r0, 2
    bne r1, r4, bad

    addi r2, r8, 0
    jal r15, app_ipc_close
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xe1
    jal r15, app_exit
