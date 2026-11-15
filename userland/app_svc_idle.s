# Windowless service: EXIT(0) on CLOSE event; otherwise yield loop.
.isa 4
.global user_init

user_init:
    addi r9, r0, 0
    addi r12, r0, 256
loop:
    addi r1, r0, 27
    ecall
    addi r2, r0, 8
    beq r1, r2, stop
    addi r1, r0, 10
    ecall
    addi r9, r9, 1
    cmplt r3, r9, r12
    bne r3, r0, loop
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt
stop:
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt
