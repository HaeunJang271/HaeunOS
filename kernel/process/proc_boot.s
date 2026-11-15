# Boot: page tables for 3 slots, init PCB0, timer, enter user init.
.isa 4
.global _start
.global arm_timer

_start:
    jal r1, p_tv
    .word 0x00100000
p_tv:
    lw r1, 0(r1)
    csrrw r0, 1, r1
    # L1_0@0x2000 L0_0@0x3000; L1_1@0x6000 L0_1@0x7000; L1_2@0x8000 L0_2@0x9000
    jal r15, map_slot0
    jal r15, map_slot1
    jal r15, map_slot2
    jal r15, init_pcb0
    jal r1, p_cu
    .word 0x0000A400
p_cu:
    lw r1, 0(r1)
    sw r0, 0(r1)
    jal r1, p_np
    .word 0x0000A404
p_np:
    lw r1, 0(r1)
    addi r2, r0, 2
    sw r2, 0(r1)
    jal r1, p_pt
    .word 0x00002001
p_pt:
    lw r1, 0(r1)
    csrrw r0, 6, r1
    sfence
    jal r1, p_irq
    .word 0x10000000
p_irq:
    lw r1, 0(r1)
    addi r2, r0, 1
    sw r2, 4(r1)
    jal r15, arm_timer
    addi r1, r0, 2
    csrrw r0, 0, r1
    jal r8, p_pcb
    .word 0x0000A000
p_pcb:
    lw r8, 0(r8)
    lw r1, 12(r8)
    csrrw r0, 2, r1
    lw r13, 36(r8)
    lw r14, 40(r8)
    eret

map_slot0:
    addi r12, r15, 0
    jal r1, p_l1
    .word 0x00002000
p_l1:
    lw r8, 0(r1)
    jal r1, p_l0
    .word 0x00003001
p_l0:
    lw r1, 0(r1)
    sw r1, 0(r8)
    jal r1, p_l0b
    .word 0x00003000
p_l0b:
    lw r9, 0(r1)
    jal r1, p_s0
    .word 0x00113017
p_s0:
    lw r10, 0(r1)
    # user stack at VA 0x00113000 -> L0 index 0x113
    sw r10, 0x44c(r9)
    jal r15, fill_k
    jal r1, p_uca
    .word 0x0010801b
p_uca:
    lw r1, 0(r1)
    sw r1, 0x420(r9)
    jal r1, p_mmio
    .word 0x00004001
p_mmio:
    lw r1, 0(r1)
    sw r1, 0x100(r8)
    jal r1, p_l0m
    .word 0x00004000
p_l0m:
    lw r3, 0(r1)
    jal r1, p_iq
    .word 0x10000007
p_iq:
    lw r1, 0(r1)
    sw r1, 0(r3)
    jal r1, p_tm
    .word 0x10001007
p_tm:
    lw r1, 0(r1)
    sw r1, 4(r3)
    jal r1, p_ua
    .word 0x10002007
p_ua:
    lw r1, 0(r1)
    sw r1, 8(r3)
    jalr r0, r12, 0

map_slot1:
    addi r12, r15, 0
    jal r1, p_l11
    .word 0x00006000
p_l11:
    lw r8, 0(r1)
    jal r1, p_l01
    .word 0x00007001
p_l01:
    lw r1, 0(r1)
    sw r1, 0(r8)
    jal r1, p_l0b1
    .word 0x00007000
p_l0b1:
    lw r9, 0(r1)
    jal r1, p_s1
    .word 0x00114017
p_s1:
    lw r10, 0(r1)
    sw r10, 0x450(r9)
    jal r15, fill_u
    jal r1, p_m1
    .word 0x00004001
p_m1:
    lw r1, 0(r1)
    sw r1, 0x100(r8)
    jalr r0, r12, 0

map_slot2:
    addi r12, r15, 0
    jal r1, p_l12
    .word 0x00008000
p_l12:
    lw r8, 0(r1)
    jal r1, p_l02
    .word 0x00009001
p_l02:
    lw r1, 0(r1)
    sw r1, 0(r8)
    jal r1, p_l0b2
    .word 0x00009000
p_l0b2:
    lw r9, 0(r1)
    jal r1, p_s2
    .word 0x00115017
p_s2:
    lw r10, 0(r1)
    sw r10, 0x454(r9)
    jal r15, fill_u
    jal r1, p_m2
    .word 0x00004001
p_m2:
    lw r1, 0(r1)
    sw r1, 0x100(r8)
    jalr r0, r12, 0

fill_u:
    addi r11, r15, 0
    jal r15, fill_k
    jal r1, p_fu
    .word 0x0010801b
p_fu:
    lw r1, 0(r1)
    sw r1, 0x420(r9)
    jalr r0, r11, 0

# Map kernel RX pages 0x00100000..0x00107000 into L0 at r9 (no U).
fill_k:
    addi r10, r15, 0
    jal r1, p_a0
    .word 0x0010000b
p_a0:
    lw r1, 0(r1)
    sw r1, 0x400(r9)
    jal r1, p_a1
    .word 0x0010100b
p_a1:
    lw r1, 0(r1)
    sw r1, 0x404(r9)
    jal r1, p_a2
    .word 0x0010200b
p_a2:
    lw r1, 0(r1)
    sw r1, 0x408(r9)
    jal r1, p_a3
    .word 0x0010300b
p_a3:
    lw r1, 0(r1)
    sw r1, 0x40c(r9)
    jal r1, p_a4
    .word 0x0010400b
p_a4:
    lw r1, 0(r1)
    sw r1, 0x410(r9)
    jal r1, p_a5
    .word 0x0010500b
p_a5:
    lw r1, 0(r1)
    sw r1, 0x414(r9)
    jal r1, p_a6
    .word 0x0010600b
p_a6:
    lw r1, 0(r1)
    sw r1, 0x418(r9)
    jal r1, p_a7
    .word 0x0010700b
p_a7:
    lw r1, 0(r1)
    sw r1, 0x41c(r9)
    # PCB page 0x0000A000 (RW kernel); L0 index VA[21:12]=0x00A → offset 0x28
    jal r1, p_pcbpg
    .word 0x0000A007
p_pcbpg:
    lw r1, 0(r1)
    sw r1, 0x28(r9)
    # Kernel stacks 0x00124000..0x00126000
    jal r1, p_ks0
    .word 0x00124007
p_ks0:
    lw r1, 0(r1)
    sw r1, 0x490(r9)
    jal r1, p_ks1
    .word 0x00125007
p_ks1:
    lw r1, 0(r1)
    sw r1, 0x494(r9)
    jal r1, p_ks2
    .word 0x00126007
p_ks2:
    lw r1, 0(r1)
    sw r1, 0x498(r9)
    jalr r0, r10, 0

init_pcb0:
    addi r11, r15, 0
    jal r8, p_p0
    .word 0x0000A000
p_p0:
    lw r8, 0(r8)
    addi r1, r0, 2
    sw r1, 0(r8)
    addi r1, r0, 1
    sw r1, 4(r8)
    sw r0, 8(r8)
    jal r1, p_ent
    .word 0x00108000
p_ent:
    lw r1, 0(r1)
    sw r1, 12(r8)
    sw r0, 16(r8)
    sw r0, 20(r8)
    sw r0, 24(r8)
    jal r1, p_pt0
    .word 0x00002001
p_pt0:
    lw r1, 0(r1)
    sw r1, 28(r8)
    jal r1, p_ks
    .word 0x00124000
p_ks:
    lw r1, 0(r1)
    sw r1, 32(r8)
    jal r1, p_us
    .word 0x00114000
p_us:
    lw r1, 0(r1)
    sw r1, 36(r8)
    jal r1, p_ub
    .word 0x00113000
p_ub:
    lw r1, 0(r1)
    sw r1, 40(r8)
    sw r0, 44(r8)
    sw r0, 48(r8)
    sw r0, 52(r8)
    sw r0, 56(r8)
    # clear other PCBs
    addi r7, r8, 0x100
    sw r0, 0(r7)
    addi r7, r7, 0x100
    sw r0, 0(r7)
    addi r7, r7, 0x100
    sw r0, 0(r7)
    jalr r0, r11, 0
