# FORK; child page-faults; parent WAIT then EXIT 0.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    addi r1, r0, 7
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt
child:
    addi r1, r0, 0
    lw r1, 0x2000(r1)
    halt
