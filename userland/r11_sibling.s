# Parent EXIT 6 while the child keeps yielding.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    addi r1, r0, 0
    addi r2, r0, 6
    ecall
    halt
child:
    addi r1, r0, 10
    ecall
    jal r0, child
