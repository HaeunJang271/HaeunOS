# User processes: init forks CPU-bound and I/O-bound children.
.isa 4
.global user_init
.global user_cpu
.global user_io

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x49
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_cpu
    addi r1, r0, 5
    ecall
    beq r1, r0, child_io
wait_loop:
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child_cpu:
    jal r0, user_cpu
child_io:
    jal r0, user_io

user_cpu:
    addi r1, r0, 9
    ecall
    addi r8, r1, 0
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r9, r1, 0
    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 0x30
    add r3, r3, r8
    ecall
    addi r4, r0, 24
cpu_loop:
    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 0x43
    ecall
    addi r4, r4, -1
    bne r4, r0, cpu_loop
    addi r1, r0, 0
    addi r2, r0, 0x63
    ecall
    halt

user_io:
    addi r1, r0, 9
    ecall
    addi r8, r1, 0
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r9, r1, 0
    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 0x30
    add r3, r3, r8
    ecall
    addi r4, r0, 8
io_loop:
    addi r1, r0, 8
    addi r2, r0, 16
    ecall
    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 0x53
    ecall
    addi r4, r4, -1
    bne r4, r0, io_loop
    addi r1, r0, 0
    addi r2, r0, 0x73
    ecall
    halt
