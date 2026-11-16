# M25.4: child deep-decrements SP into guard; parent survives (O/P/Z).
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4F
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_ovf
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x50
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child_ovf:
    # Burn the 4 KiB stack then one more word into the guard page.
    addi r2, r0, 0
    addi r3, r0, 1025
burn:
    beq r2, r3, done_burn
    addi r13, r13, -4
    sw r2, 0(r13)
    addi r2, r2, 1
    jal r0, burn
done_burn:
    addi r1, r0, 0
    addi r2, r0, 0xE3
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
