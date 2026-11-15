# Word load/store helpers and PMM arena bounds (fixed physical window).
.isa 4
.global mem_load
.global mem_store
.global pmm_base
.global pmm_end
.global pmm_meta

# mem_load(addr) -> word at addr
mem_load:
    lw r1, 0(r2)
    jalr r0, r15, 0

# mem_store(addr, value) -> 0
mem_store:
    sw r3, 0(r2)
    addi r1, r0, 0
    jalr r0, r15, 0

# Arena [0x00400000, 0x00800000): 1024 pages of 4 KiB.
pmm_base:
    jal r1, p_base
    .word 0x00400000
p_base:
    lw r1, 0(r1)
    jalr r0, r15, 0

pmm_end:
    jal r1, p_end
    .word 0x00800000
p_end:
    lw r1, 0(r1)
    jalr r0, r15, 0

# Freelist head + ready flag words at 0x003FF000.
pmm_meta:
    jal r1, p_meta
    .word 0x003FF000
p_meta:
    lw r1, 0(r1)
    jalr r0, r15, 0
