# Guest block device bread/bwrite (512-byte DMA at 0x00009000).
.isa 4
.global blk_read
.global blk_write

# blk_read(lba in r2): copies block to 0x9000, returns 0 or 1 on error.
blk_read:
    addi r7, r15, 0
    jal r1, p_blk
    .word 0x10003000
p_blk:
    lw r9, 0(r1)
    jal r1, p_dma
    .word 0x00009000
p_dma:
    lw r10, 0(r1)
    sw r2, 8(r9)
    sw r10, 12(r9)
    addi r1, r0, 1
    sw r1, 4(r9)
    lw r1, 0(r9)
    addi r2, r0, 2
    bne r1, r2, br_err
    addi r1, r0, 0
    jalr r0, r7, 0
br_err:
    addi r1, r0, 1
    jalr r0, r7, 0

# blk_write(lba in r2): writes 0x9000 to LBA.
blk_write:
    addi r7, r15, 0
    jal r1, p_blk2
    .word 0x10003000
p_blk2:
    lw r9, 0(r1)
    jal r1, p_dma2
    .word 0x00009000
p_dma2:
    lw r10, 0(r1)
    sw r2, 8(r9)
    sw r10, 12(r9)
    addi r1, r0, 2
    sw r1, 4(r9)
    lw r1, 0(r9)
    addi r2, r0, 2
    bne r1, r2, bw_err
    addi r1, r0, 0
    jalr r0, r7, 0
bw_err:
    addi r1, r0, 1
    jalr r0, r7, 0
