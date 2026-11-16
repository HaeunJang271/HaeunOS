# M24.2 orphan/WAIT: child exits before parent WAIT; second after WAIT.
# B=before first WAIT, 1/2=reaped, D=done.
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
    beq r1, r0, early_child
    addi r1, r0, 10
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x42
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x31
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, late_child
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x32
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x44
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

early_child:
    addi r1, r0, 0
    addi r2, r0, 11
    ecall
    halt

late_child:
    addi r1, r0, 0
    addi r2, r0, 22
    ecall
    halt
