# SDK runtime: spawn every registered app in order, wait, then EXIT.
.isa 4
.global user_init

user_init:
    addi r1, r0, 24
    ecall
    addi r9, r1, 0
    addi r8, r0, 0
loop:
    cmplt r1, r8, r9
    beq r1, r0, done
    addi r1, r0, 26
    addi r2, r8, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    jal r15, yield_burst
    addi r1, r0, 7
    ecall
    addi r8, r8, 1
    jal r0, loop
done:
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt
fail:
    addi r1, r0, 0
    addi r2, r0, 0xe1
    ecall
    halt

yield_burst:
    addi r10, r15, 0
    addi r11, r0, 48
yb:
    addi r1, r0, 10
    ecall
    addi r11, r11, -1
    bne r11, r0, yb
    jalr r0, r10, 0
