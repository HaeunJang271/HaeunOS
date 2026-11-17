# Parent and child alternate via YIELD; parent EXIT 24 last.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 5
    ecall
    beq r1, r0, child
    addi r8, r0, 30
p_loop:
    addi r1, r0, 10
    ecall
    addi r8, r8, -1
    bne r8, r0, p_loop
    addi r1, r0, 0
    addi r2, r0, 24
    ecall
    halt
child:
    addi r8, r0, 80
c_loop:
    addi r1, r0, 10
    ecall
    addi r8, r8, -1
    bne r8, r0, c_loop
    addi r1, r0, 0
    addi r2, r0, 25
    ecall
    halt
