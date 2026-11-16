# Orphan policy: on EXIT/fault, reparent children to init (PCB0.pid).
# Live and ZOMBIE children keep running/waiting; init may WAIT zombies.
# Desktop SPAWN-time memset remains for apps EXIT'd without WAIT.
.isa 4
.global orphan_reparent

# r8 = exiting PCB. Clobbers r1-r7,r9-r11. Preserves r8,r15 via r11.
orphan_reparent:
    addi r11, r15, 0
    lw r9, 4(r8)
    jal r1, p_base
    .word 0x0000A000
p_base:
    lw r7, 0(r1)
    lw r10, 4(r7)
    addi r6, r0, 0
    addi r5, r7, 0
or_loop:
    addi r1, r0, 4
    beq r6, r1, or_done
    beq r5, r8, or_next
    lw r1, 8(r5)
    bne r1, r9, or_next
    lw r1, 0(r5)
    beq r1, r0, or_next
    sw r10, 8(r5)
or_next:
    addi r5, r5, 0x100
    addi r6, r6, 1
    jal r0, or_loop
or_done:
    jalr r0, r11, 0
