# Desktop launcher: spawn calc, wait, spawn edit, wait, spawn files, wait, exit.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    addi r1, r0, 18
    addi r2, r0, 2
    addi r3, r0, 2
    addi r4, r0, 28
    addi r5, r0, 10
    addi r6, r0, 0x1C000
    ecall
    addi r8, r1, 0
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, col
    .word 0x00202040
col:
    lw r3, 0(r3)
    ecall

    addi r1, r0, 24
    ecall
    addi r9, r1, 0
    addi r2, r0, 3
    cmplt r3, r9, r2
    bne r3, r0, fail

    # --- calc ---
    addi r1, r0, 26
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_burst
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_burst
    addi r1, r0, 7
    ecall

    # --- edit ---
    addi r1, r0, 26
    addi r2, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_burst
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_burst
    addi r1, r0, 7
    ecall

    # --- files ---
    addi r1, r0, 26
    addi r2, r0, 2
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_burst
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_burst
    addi r1, r0, 7
    ecall

    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    addi r1, r0, 19
    addi r2, r8, 0
    ecall
    # EXIT with code 1 (launcher success)
    addi r2, r0, 1
    addi r1, r0, 0
    ecall
    halt

yield_burst:
    addi r10, r15, 0
    addi r11, r0, 64
yb:
    addi r1, r0, 10
    ecall
    addi r11, r11, -1
    bne r11, r0, yb
    jalr r0, r10, 0

fail:
    addi r1, r0, 0
    addi r2, r0, 0xe0
    ecall
    halt
