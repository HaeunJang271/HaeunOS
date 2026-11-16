# M25.6: FORK user memory — copy parent code + ustack + heap into child AS,
# remap child code/heap PTEs, relocate r13/r14 into child stack VA range.
# No COW. r6=child slot, r7=child PCB, r8=parent PCB.
.isa 4
.global fork_user_mem

fork_user_mem:
    addi r11, r15, 0
    # parent slot = (pcb - 0xA000) / 0x100 → r12
    jal r1, p_base
    .word 0x0000A000
p_base:
    lw r1, 0(r1)
    sub r2, r8, r1
    addi r3, r0, 0
    addi r4, r0, 0x100
ps_div:
    cmplt r5, r2, r4
    bne r5, r0, ps_got
    sub r2, r2, r4
    addi r3, r3, 1
    jal r0, ps_div
ps_got:
    addi r12, r3, 0
    # --- code page: parent VA 0x108000 → child code PA ---
    addi r3, r6, 0
    jal r15, code_pa
    addi r10, r1, 0
    jal r1, p_src
    .word 0x00108000
p_src:
    lw r9, 0(r1)
    addi r2, r0, 0
    addi r4, r0, 4096
cp_loop:
    beq r2, r4, cp_done
    add r1, r9, r2
    lw r5, 0(r1)
    add r1, r10, r2
    sw r5, 0(r1)
    addi r2, r2, 4
    jal r0, cp_loop
cp_done:
    # child L0 user-code PTE @ +0x420 → child_code|0x1b (U|RX)
    addi r3, r6, 0
    jal r15, l0_pa
    addi r1, r1, 0x420
    addi r2, r10, 0
    addi r5, r0, 0x1b
    or r2, r2, r5
    sw r2, 0(r1)
    # --- ustack page: parent PA → child PA (identity-mapped) ---
    addi r3, r12, 0
    jal r15, ustack_pa
    addi r9, r1, 0
    addi r3, r6, 0
    jal r15, ustack_pa
    addi r10, r1, 0
    addi r2, r0, 0
    addi r4, r0, 4096
st_loop:
    beq r2, r4, st_done
    add r1, r9, r2
    lw r5, 0(r1)
    add r1, r10, r2
    sw r5, 0(r1)
    addi r2, r2, 4
    jal r0, st_loop
st_done:
    # --- heap page: parent VA 0x109000 → child heap PA ---
    addi r3, r6, 0
    jal r15, heap_pa
    addi r10, r1, 0
    jal r1, p_hs
    .word 0x00109000
p_hs:
    lw r9, 0(r1)
    addi r2, r0, 0
    addi r4, r0, 4096
hp_loop:
    beq r2, r4, hp_done
    add r1, r9, r2
    lw r5, 0(r1)
    add r1, r10, r2
    sw r5, 0(r1)
    addi r2, r2, 4
    jal r0, hp_loop
hp_done:
    # child L0 heap PTE @ +0x424 → child_heap|0x17 (U|RW, no X)
    addi r3, r6, 0
    jal r15, l0_pa
    addi r1, r1, 0x424
    addi r2, r10, 0
    addi r5, r0, 0x17
    or r2, r2, r5
    sw r2, 0(r1)
    # relocate saved r13 (PCB+116) into child stack VA
    addi r4, r0, 116
    jal r15, reloc_sp
    addi r4, r0, 120
    jal r15, reloc_sp
    sfence
    jalr r0, r11, 0

# r4 = PCB byte offset of SP-like reg; parent r8 / child r7 bounds at +36/+40
reloc_sp:
    addi r9, r15, 0
    add r1, r7, r4
    lw r1, 0(r1)
    lw r2, 40(r8)
    lw r3, 36(r8)
    cmplt r5, r1, r2
    bne r5, r0, rs_fallback
    # r1 > top? (cmplt top, r1)
    cmplt r5, r3, r1
    bne r5, r0, rs_fallback
    lw r5, 40(r7)
    sub r1, r1, r2
    add r1, r5, r1
    add r2, r7, r4
    sw r1, 0(r2)
    jalr r0, r9, 0
rs_fallback:
    addi r3, r0, 120
    beq r4, r3, rs_base
    lw r1, 36(r7)
    add r2, r7, r4
    sw r1, 0(r2)
    jalr r0, r9, 0
rs_base:
    lw r1, 40(r7)
    add r2, r7, r4
    sw r1, 0(r2)
    jalr r0, r9, 0

# r3=slot → r1=code PA
code_pa:
    beq r3, r0, c0
    addi r1, r0, 1
    beq r3, r1, c1
    jal r1, pc2
    .word 0x0010D000
pc2:
    lw r1, 0(r1)
    jalr r0, r15, 0
c0:
    jal r1, pc0
    .word 0x00108000
pc0:
    lw r1, 0(r1)
    jalr r0, r15, 0
c1:
    jal r1, pc1
    .word 0x0010C000
pc1:
    lw r1, 0(r1)
    jalr r0, r15, 0

# r3=slot → r1=L0 base PA
l0_pa:
    beq r3, r0, l0
    addi r1, r0, 1
    beq r3, r1, l1
    jal r1, pl2
    .word 0x00009000
pl2:
    lw r1, 0(r1)
    jalr r0, r15, 0
l0:
    jal r1, pl0
    .word 0x00003000
pl0:
    lw r1, 0(r1)
    jalr r0, r15, 0
l1:
    jal r1, pl1
    .word 0x00007000
pl1:
    lw r1, 0(r1)
    jalr r0, r15, 0

# r3=slot → r1=ustack page PA
ustack_pa:
    beq r3, r0, u0
    addi r1, r0, 1
    beq r3, r1, u1
    jal r1, pu2
    .word 0x00115000
pu2:
    lw r1, 0(r1)
    jalr r0, r15, 0
u0:
    jal r1, pu0
    .word 0x00113000
pu0:
    lw r1, 0(r1)
    jalr r0, r15, 0
u1:
    jal r1, pu1
    .word 0x00114000
pu1:
    lw r1, 0(r1)
    jalr r0, r15, 0

# r3=slot → r1=heap PA
heap_pa:
    beq r3, r0, h0
    addi r1, r0, 1
    beq r3, r1, h1
    jal r1, ph2
    .word 0x00110000
ph2:
    lw r1, 0(r1)
    jalr r0, r15, 0
h0:
    jal r1, ph0
    .word 0x0010E000
ph0:
    lw r1, 0(r1)
    jalr r0, r15, 0
h1:
    jal r1, ph1
    .word 0x0010F000
ph1:
    lw r1, 0(r1)
    jalr r0, r15, 0
