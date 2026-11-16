# M25.6: raw store/load at heap VA; markers M Z
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    jal r1, p_hv
    .word 0x00109000
p_hv:
    lw r1, 0(r1)
    jal r2, p_tag
    .word 0x12480001
p_tag:
    lw r2, 0(r2)
    sw r2, 0(r1)
    lw r3, 0(r1)
    bne r2, r3, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4D
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
