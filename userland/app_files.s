# File manager: APP_COUNT + APP_NAME listing, paint window, EXIT.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    addi r1, r0, 18
    addi r2, r0, 16
    addi r3, r0, 8
    addi r4, r0, 24
    addi r5, r0, 12
    jal r6, base
    .word 0x00024000
base:
    lw r6, 0(r6)
    ecall
    addi r8, r1, 0

    addi r1, r0, 24
    ecall
    addi r9, r1, 0
    beq r9, r0, fail

    # fetch name of app 0 into stack buffer
    addi r1, r0, 25
    addi r2, r0, 0
    addi r3, r13, -32
    addi r4, r0, 16
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail

    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, ok
    .word 0x00804000
ok:
    lw r3, 0(r3)
    ecall
    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    addi r1, r0, 19
    addi r2, r8, 0
    ecall
    lw r2, -4(r13)
    addi r1, r0, 0
    ecall
    halt
fail:
    addi r1, r0, 0
    addi r2, r0, 0xc3
    ecall
    halt
