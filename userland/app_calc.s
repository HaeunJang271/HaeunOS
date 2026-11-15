# Calculator app: evaluates 12+34 into window, then EXIT.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    # WIN_CREATE (8,4) 24x12 @0x20000
    addi r1, r0, 18
    addi r2, r0, 8
    addi r3, r0, 4
    addi r4, r0, 24
    addi r5, r0, 12
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    ecall
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    # fill dark
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, bg
    .word 0x00101010
bg:
    lw r3, 0(r3)
    ecall

    # 12 + 34 = 46 (no KBD MMIO — process-v1 does not U-map input devices)
    addi r4, r0, 12
    addi r5, r0, 34
    add r6, r4, r5
    addi r7, r0, 46
    bne r6, r7, bad_math

    # success: green fill
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, okc
    .word 0x0000C000
okc:
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
bad_math:
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, erc
    .word 0x000000C0
erc:
    lw r3, 0(r3)
    ecall
bad:
    addi r1, r0, 0
    addi r2, r0, 0xc1
    ecall
    halt
