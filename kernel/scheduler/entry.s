# Boot: identity maps, TCBs, timer arm, enter task 0. Trap/users in sched.s.
.isa 4
.global _start

_start:
    jal r1, p_trapvec
    .word 0x00100000
p_trapvec:
    lw r1, 0(r1)
    csrrw r0, 1, r1
    jal r1, p_l1
    .word 0x00002000
p_l1:
    lw r3, 0(r1)
    jal r1, p_l0p
    .word 0x00003001
p_l0p:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_l1u
    .word 0x00002100
p_l1u:
    lw r3, 0(r1)
    jal r1, p_l0u
    .word 0x00004001
p_l0u:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_a
    .word 0x00003400
p_a:
    lw r3, 0(r1)
    jal r1, p_b
    .word 0x0010000b
p_b:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_c
    .word 0x00003404
p_c:
    lw r3, 0(r1)
    jal r1, p_d
    .word 0x0010101b
p_d:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_e
    .word 0x0000340c
p_e:
    lw r3, 0(r1)
    jal r1, p_f
    .word 0x00103017
p_f:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_g
    .word 0x00003410
p_g:
    lw r3, 0(r1)
    jal r1, p_h
    .word 0x00104017
p_h:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_i
    .word 0x00003014
p_i:
    lw r3, 0(r1)
    jal r1, p_j
    .word 0x00005007
p_j:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_irqpte
    .word 0x00004000
p_irqpte:
    lw r3, 0(r1)
    jal r1, p_irqval
    .word 0x10000007
p_irqval:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_tmrpte
    .word 0x00004004
p_tmrpte:
    lw r3, 0(r1)
    jal r1, p_tmrval
    .word 0x10001007
p_tmrval:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r1, p_k
    .word 0x00004008
p_k:
    lw r3, 0(r1)
    jal r1, p_l
    .word 0x10002007
p_l:
    lw r2, 0(r1)
    sw r2, 0(r3)
    jal r8, p_t0
    .word 0x00005000
p_t0:
    lw r8, 0(r8)
    addi r1, r0, 1
    sw r1, 0(r8)
    jal r1, p_ea
    .word 0x00101000
p_ea:
    lw r1, 0(r1)
    sw r1, 4(r8)
    jal r1, p_s0t
    .word 0x00104000
p_s0t:
    lw r1, 0(r1)
    sw r1, 56(r8)
    jal r1, p_s0b
    .word 0x00103000
p_s0b:
    lw r1, 0(r1)
    sw r1, 60(r8)
    jal r8, p_t1
    .word 0x00005080
p_t1:
    lw r8, 0(r8)
    addi r1, r0, 1
    sw r1, 0(r8)
    jal r1, p_eb
    .word 0x00101028
p_eb:
    lw r1, 0(r1)
    sw r1, 4(r8)
    jal r1, p_s1t
    .word 0x00105000
p_s1t:
    lw r1, 0(r1)
    sw r1, 56(r8)
    jal r1, p_s1b
    .word 0x00104000
p_s1b:
    lw r1, 0(r1)
    sw r1, 60(r8)
    jal r1, p_cu
    .word 0x00005100
p_cu:
    lw r1, 0(r1)
    sw r0, 0(r1)
    jal r1, p_ptbr
    .word 0x00002001
p_ptbr:
    lw r1, 0(r1)
    csrrw r0, 6, r1
    jal r1, p_irqc
    .word 0x10000000
p_irqc:
    lw r1, 0(r1)
    addi r2, r0, 1
    sw r2, 4(r1)
    jal r1, p_tmr
    .word 0x10001000
p_tmr:
    lw r1, 0(r1)
    lw r2, 0(r1)
    addi r2, r2, 32
    sw r2, 8(r1)
    addi r2, r0, 1
    sw r2, 16(r1)
    addi r1, r0, 2
    csrrw r0, 0, r1
    jal r8, p_t0r
    .word 0x00005000
p_t0r:
    lw r8, 0(r8)
    lw r1, 4(r8)
    csrrw r0, 2, r1
    lw r13, 56(r8)
    lw r14, 60(r8)
    eret
