# R2.0/R3.0/R3.4 scheduling policy. Selection only — no PCB/curr/PTBR/timer writes.
# SCHED_POLICY_RR=0, SCHED_POLICY_FCFS=1 at 0xA474. Other IDs → no-candidate.
# R3.4: Preemptive FIFO by 64-bit ready_seq (+172/+176). Tie → lower slot.
.isa 4
.global policy_select
.global policy_rr
.global policy_fcfs
.global sched_slots
.global sched_pol_boot

# Input: r13 = current PCB index.
# Output: r1=1 and r6=selected slot, or r1=0. Preserves r11,r13.
policy_select:
    addi r12, r15, 0
    jal r1, ps_id
    .word 0x0000A474
ps_id:
    lw r1, 0(r1)
    lw r1, 0(r1)
    beq r1, r0, policy_rr
    addi r2, r0, 1
    beq r1, r2, policy_fcfs
    addi r1, r0, 0
    jalr r0, r12, 0

# Round-robin: scan curr+1 .. wrap; READY or RUNNING.
policy_rr:
    addi r6, r13, 0
    addi r5, r0, 0
    jal r15, sched_slots
    addi r4, r1, 0
pr_scan:
    addi r6, r6, 1
    cmplt r2, r6, r4
    bne r2, r0, pr_okidx
    addi r6, r0, 0
pr_okidx:
    jal r1, pr_pb
    .word 0x0000A000
pr_pb:
    lw r8, 0(r1)
    addi r2, r0, 0
pr_mul:
    beq r2, r6, pr_have
    addi r8, r8, 0x100
    addi r2, r2, 1
    jal r0, pr_mul
pr_have:
    lw r1, 0(r8)
    addi r2, r0, 1
    beq r1, r2, pr_yes
    addi r2, r0, 2
    beq r1, r2, pr_yes
    addi r5, r5, 1
    beq r5, r4, pr_no
    jal r0, pr_scan
pr_yes:
    addi r1, r0, 1
    jalr r0, r12, 0
pr_no:
    addi r1, r0, 0
    jalr r0, r12, 0

# Preemptive FIFO by ready_seq_lo/hi (+172/+176). Older unsigned 64-bit wins.
policy_fcfs:
    jal r15, sched_slots
    addi r4, r1, 0
    addi r6, r0, 0
    addi r5, r0, 0
    addi r3, r0, 0
    addi r7, r0, 0
    addi r10, r0, 0
pf_scan:
    beq r3, r4, pf_done
    jal r1, pf_pb
    .word 0x0000A000
pf_pb:
    lw r8, 0(r1)
    addi r2, r0, 0
pf_mul:
    beq r2, r3, pf_have
    addi r8, r8, 0x100
    addi r2, r2, 1
    jal r0, pf_mul
pf_have:
    lw r1, 0(r8)
    addi r2, r0, 1
    beq r1, r2, pf_cand
    addi r2, r0, 2
    beq r1, r2, pf_cand
    jal r0, pf_next
pf_cand:
    lw r1, 172(r8)
    lw r9, 176(r8)
    beq r5, r0, pf_take
    # unsigned (r9:r1) < (r10:r7)?
    jal r2, pf_sgn
    .word -2147483648
pf_sgn:
    lw r2, 0(r2)
    xor r14, r9, r2
    xor r8, r10, r2
    cmplt r14, r14, r8
    bne r14, r0, pf_take
    bne r9, r10, pf_next
    xor r14, r1, r2
    xor r8, r7, r2
    cmplt r14, r14, r8
    bne r14, r0, pf_take
    bne r1, r7, pf_next
    cmplt r2, r3, r6
    beq r2, r0, pf_next
pf_take:
    addi r6, r3, 0
    addi r7, r1, 0
    addi r10, r9, 0
    addi r5, r0, 1
pf_next:
    addi r3, r3, 1
    jal r0, pf_scan
pf_done:
    beq r5, r0, pf_no
    addi r1, r0, 1
    jalr r0, r12, 0
pf_no:
    addi r1, r0, 0
    jalr r0, r12, 0

.global sched_pol_init

sched_pol_init:
    addi r10, r15, 0
    jal r1, spi_go
sched_pol_boot:
    .word 0
spi_go:
    lw r2, 0(r1)
    jal r1, spi_dst
    .word 0x0000A474
spi_dst:
    lw r1, 0(r1)
    sw r2, 0(r1)
    jalr r0, r10, 0
