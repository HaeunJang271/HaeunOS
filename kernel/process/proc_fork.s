# FORK and WAIT implementations.
.isa 4
.global sys_fork
.global sys_wait
.global sys_trywait
.global advance_epc
.global restore_ret
.global pick_next
.global fork_user_mem

sys_fork:
    jal r15, advance_epc
    jal r1, p_pb
    .word 0x0000A000
p_pb:
    lw r7, 0(r1)
    addi r6, r0, 0
fk_scan:
    # Cap at HAEUN_APP_SLOTS (3): PCB3 would alias AS2 (no fourth AS).
    addi r1, r0, 3
    beq r6, r1, fk_fail
    lw r1, 0(r7)
    beq r1, r0, fk_got
    addi r7, r7, 0x100
    addi r6, r6, 1
    jal r0, fk_scan
fk_got:
    # r7 = free PCB, r8 = parent, r6 = slot (0..2)
    jal r1, p_np
    .word 0x0000A404
p_np:
    lw r1, 0(r1)
    lw r2, 0(r1)
    sw r2, 4(r7)
    addi r2, r2, 1
    sw r2, 0(r1)
    lw r1, 4(r8)
    sw r1, 8(r7)
    # Stay FREE until memory clone completes (no schedulable partial child).
    sw r0, 0(r7)
    lw r1, 12(r8)
    sw r1, 12(r7)
    sw r0, 16(r7)
    sw r0, 20(r7)
    sw r0, 24(r7)
    # per-slot address space + stacks
    jal r15, fork_stacks
    # copy saved GPRs
    addi r3, r0, 64
fk_copy:
    addi r1, r0, 128
    beq r3, r1, fk_cdone
    add r4, r8, r3
    add r5, r7, r3
    lw r1, 0(r4)
    sw r1, 0(r5)
    addi r3, r3, 4
    jal r0, fk_copy
fk_cdone:
    # M25.3: clone user code + ustack; relocate SP into child VA.
    jal r15, fork_user_mem
    addi r1, r0, 1
    sw r1, 0(r7)
    sw r0, 64(r7)
    lw r1, 4(r7)
    sw r1, 64(r8)
    # open console on child fd0
    addi r1, r0, 1
    sw r1, 44(r7)
    sw r0, 48(r7)
    sw r0, 52(r7)
    sw r0, 56(r7)
    jal r0, restore_ret
fk_fail:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r0, restore_ret

# fork_stacks: set k/u stacks for child slot r6 into PCB r7
fork_stacks:
    addi r10, r15, 0
    beq r6, r0, fs0
    addi r1, r0, 1
    beq r6, r1, fs1
    jal r0, fs2
fs0:
    jal r1, p_pt0
    .word 0x00002001
p_pt0:
    lw r1, 0(r1)
    sw r1, 28(r7)
    jal r1, p_k0
    .word 0x00124000
p_k0:
    lw r1, 0(r1)
    sw r1, 32(r7)
    jal r1, p_u0t
    .word 0x00114000
p_u0t:
    lw r1, 0(r1)
    sw r1, 36(r7)
    sw r1, 116(r7)
    jal r1, p_u0b
    .word 0x00113000
p_u0b:
    lw r1, 0(r1)
    sw r1, 40(r7)
    sw r1, 120(r7)
    jalr r0, r10, 0
fs1:
    jal r1, p_pt1
    .word 0x00006001
p_pt1:
    lw r1, 0(r1)
    sw r1, 28(r7)
    jal r1, p_k1
    .word 0x00125000
p_k1:
    lw r1, 0(r1)
    sw r1, 32(r7)
    jal r1, p_u1t
    .word 0x00115000
p_u1t:
    lw r1, 0(r1)
    sw r1, 36(r7)
    sw r1, 116(r7)
    jal r1, p_u1b
    .word 0x00114000
p_u1b:
    lw r1, 0(r1)
    sw r1, 40(r7)
    sw r1, 120(r7)
    jalr r0, r10, 0
fs2:
    jal r1, p_pt2
    .word 0x00008001
p_pt2:
    lw r1, 0(r1)
    sw r1, 28(r7)
    jal r1, p_k2
    .word 0x00126000
p_k2:
    lw r1, 0(r1)
    sw r1, 32(r7)
    jal r1, p_u2t
    .word 0x00116000
p_u2t:
    lw r1, 0(r1)
    sw r1, 36(r7)
    sw r1, 116(r7)
    jal r1, p_u2b
    .word 0x00115000
p_u2b:
    lw r1, 0(r1)
    sw r1, 40(r7)
    sw r1, 120(r7)
    jalr r0, r10, 0

sys_wait:
    jal r1, p_pb2
    .word 0x0000A000
p_pb2:
    lw r7, 0(r1)
    addi r6, r0, 0
    lw r9, 4(r8)
wt_scan:
    addi r1, r0, 4
    beq r6, r1, wt_block
    lw r1, 0(r7)
    addi r2, r0, 5
    bne r1, r2, wt_next
    lw r1, 8(r7)
    bne r1, r9, wt_next
    lw r1, 16(r7)
    sw r1, 64(r8)
    sw r0, 0(r7)
    sw r0, 44(r7)
    sw r0, 48(r7)
    sw r0, 52(r7)
    sw r0, 56(r7)
    jal r15, advance_epc
    jal r0, restore_ret
wt_next:
    addi r7, r7, 0x100
    addi r6, r6, 1
    jal r0, wt_scan
# Block without advancing EPC so a wake re-enters WAIT and rescans zombies.
wt_block:
    sw r0, 24(r8)
    addi r1, r0, 4
    sw r1, 0(r8)
    jal r15, pick_next
    jal r0, restore_ret

# Non-blocking WAIT: reap one own ZOMBIE or return -1. Does not change WAIT.
sys_trywait:
    jal r1, p_pb3
    .word 0x0000A000
p_pb3:
    lw r7, 0(r1)
    addi r6, r0, 0
    lw r9, 4(r8)
tw_scan:
    addi r1, r0, 4
    beq r6, r1, tw_none
    lw r1, 0(r7)
    addi r2, r0, 5
    bne r1, r2, tw_next
    lw r1, 8(r7)
    bne r1, r9, tw_next
    lw r1, 16(r7)
    sw r1, 64(r8)
    sw r0, 0(r7)
    sw r0, 44(r7)
    sw r0, 48(r7)
    sw r0, 52(r7)
    sw r0, 56(r7)
    jal r15, advance_epc
    jal r0, restore_ret
tw_next:
    addi r7, r7, 0x100
    addi r6, r6, 1
    jal r0, tw_scan
tw_none:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
