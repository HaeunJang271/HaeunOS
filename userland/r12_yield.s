# Many YIELDs then EXIT 22. Self-dispatch without context switch.
.isa 4
.global user_init

.align 4096
user_init:
    addi r8, r0, 40
y_loop:
    addi r1, r0, 10
    ecall
    addi r8, r8, -1
    bne r8, r0, y_loop
    addi r1, r0, 0
    addi r2, r0, 22
    ecall
    halt
