# Spawn app0 then app1 concurrently; WAIT twice. For IPC send/recv pairs.
.isa 4
.global user_init

user_init:
    addi r1, r0, 24
    ecall
    addi r2, r0, 2
    cmplt r3, r1, r2
    bne r3, r0, fail

    addi r1, r0, 26
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail

    addi r11, r0, 64
yb0:
    addi r1, r0, 10
    ecall
    addi r11, r11, -1
    bne r11, r0, yb0

    addi r1, r0, 26
    addi r2, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail

    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall

    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt
fail:
    addi r1, r0, 0
    addi r2, r0, 0xe1
    ecall
    halt
