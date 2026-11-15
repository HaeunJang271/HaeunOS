# Foundation reset handoff: validate HBI stack, call kmain, halt with status.
.isa 4
.global _start
.global kmain

_start:
    # r2 = HBI, r3 = 32, r13 = stack top, r14 = stack base (from bootloader)
    beq r2, r0, bad_hbi
    addi r1, r0, 32
    bne r3, r1, bad_hbi
    lw r1, 0(r2)
    jal r4, p_magic
    .word 0x31494248
p_magic:
    lw r4, 0(r4)
    bne r1, r4, bad_hbi
    lw r1, 16(r2)
    bne r1, r14, bad_hbi
    lw r1, 20(r2)
    bne r1, r13, bad_hbi
    cmplt r1, r14, r13
    beq r1, r0, bad_hbi
    # Keep 16-byte alignment on the exclusive stack top.
    addi r1, r0, 15
    and r1, r13, r1
    bne r1, r0, bad_hbi
    jal r15, kmain
    halt

bad_hbi:
    addi r1, r0, 2
    halt
