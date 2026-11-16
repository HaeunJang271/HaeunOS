# M24.4 A/B/D: WAIT/TRYWAIT interaction, exit paths, repeated orphan cycles.
# Markers: S start; T trywait empty; W wait reap; N trywait after; R cycle;
# F fault parent ok; Z done. Fail → EXIT 0xEE.
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
    addi r3, r0, 0x53
    ecall
    # A0: TRYWAIT empty → -1
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x54
    ecall
    # A1: child EXIT; TRYWAIT; TRYWAIT again -1; (no double)
    addi r1, r0, 5
    ecall
    beq r1, r0, child_exit
    addi r1, r0, 10
    ecall
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    beq r1, r2, fail
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x57
    ecall
    # A2: WAIT then TRYWAIT -1
    addi r1, r0, 5
    ecall
    beq r1, r0, child_exit2
    addi r1, r0, 7
    ecall
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4E
    ecall
    # D: orphan cycles ×3 (parent exits before WAIT on child)
    addi r4, r0, 3
d_loop:
    addi r1, r0, 5
    ecall
    beq r1, r0, middle
    addi r1, r0, 7
    ecall
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    beq r1, r2, d_need_wait
    jal r0, d_mark
d_need_wait:
    addi r1, r0, 7
    ecall
d_mark:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x52
    ecall
    addi r4, r4, -1
    bne r4, r0, d_loop
    # B: fault child; parent WAIT (reuse m17 pattern)
    addi r1, r0, 5
    ecall
    beq r1, r0, child_fault
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x46
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt

child_exit:
    addi r1, r0, 0
    addi r2, r0, 11
    ecall
    halt

child_exit2:
    addi r1, r0, 0
    addi r2, r0, 12
    ecall
    halt

middle:
    addi r1, r0, 5
    ecall
    beq r1, r0, kid
    addi r1, r0, 0
    addi r2, r0, 3
    ecall
    halt

kid:
    addi r1, r0, 0
    addi r2, r0, 9
    ecall
    halt

child_fault:
    addi r1, r0, 0
    lw r1, 0x2000(r1)
    halt
