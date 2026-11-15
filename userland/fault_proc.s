# Init forks a child that page-faults; parent continues and EXITs cleanly.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 5
    ecall
    beq r1, r0, child_fault
parent:
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x50
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4F
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child_fault:
    # Unmapped user VA → page fault (cause 5); kernel kills this PCB.
    addi r1, r0, 0
    lw r1, 0x2000(r1)
    halt
