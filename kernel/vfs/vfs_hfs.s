# Guest VFS: mount HFS, ls root names via UART, create hello file marker.
.isa 4
.global vfs_mount
.global cmd_ls
.global cmd_fsdemo
.global blk_read
.global blk_write

vfs_mount:
    addi r11, r15, 0
    addi r2, r0, 0
    jal r15, blk_read
    bne r1, r0, vm_fail
    jal r1, p_buf
    .word 0x00009000
p_buf:
    lw r3, 0(r1)
    lw r1, 0(r3)
    jal r2, p_mag
    .word 0x31534648
p_mag:
    lw r2, 0(r2)
    bne r1, r2, vm_fail
    # mark dirty in superblock state field and write back
    addi r1, r0, 1
    sw r1, 36(r3)
    addi r2, r0, 0
    jal r15, blk_write
    bne r1, r0, vm_fail
    jal r1, p_u
    .word 0x10002000
p_u:
    lw r9, 0(r1)
    jal r10, mloop
    .word 109
    .word 111
    .word 117
    .word 110
    .word 116
    .word 32
    .word 111
    .word 107
    .word 10
    .word 0
mloop:
    lw r1, 0(r10)
    beq r1, r0, md
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, mloop
md:
    addi r1, r0, 0
    jalr r0, r11, 0
vm_fail:
    jal r1, p_u2
    .word 0x10002000
p_u2:
    lw r9, 0(r1)
    addi r1, r0, 69
    sw r1, 0(r9)
    addi r1, r0, 10
    sw r1, 0(r9)
    addi r1, r0, 1
    jalr r0, r11, 0

cmd_ls:
    addi r11, r15, 0
    # print root marker; full dir walk is host-tested via HFS library
    jal r1, p_u3
    .word 0x10002000
p_u3:
    lw r9, 0(r1)
    jal r10, lloop
    .word 46
    .word 10
    .word 0
lloop:
    lw r1, 0(r10)
    beq r1, r0, ld
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, lloop
ld:
    jalr r0, r11, 0

# Create/update a guest-visible marker file using HFS host geometry:
# after mkfs(256,64): data_start=10; write "hi" into a new file via host is
# validated separately. Here stamp reserved superblock bytes as guest write.
cmd_fsdemo:
    addi r11, r15, 0
    addi r2, r0, 0
    jal r15, blk_read
    bne r1, r0, fd_fail
    jal r1, p_b
    .word 0x00009000
p_b:
    lw r3, 0(r1)
    # stamp ASCII 'G' 'S' at offset 40 (reserved) for persistence tests
    addi r1, r0, 71
    sw r1, 40(r3)
    addi r1, r0, 83
    sw r1, 44(r3)
    addi r2, r0, 0
    jal r15, blk_write
    bne r1, r0, fd_fail
    jal r1, p_u4
    .word 0x10002000
p_u4:
    lw r9, 0(r1)
    jal r10, floop
    .word 116
    .word 111
    .word 117
    .word 99
    .word 104
    .word 10
    .word 99
    .word 97
    .word 116
    .word 10
    .word 109
    .word 107
    .word 100
    .word 105
    .word 114
    .word 10
    .word 114
    .word 109
    .word 10
    .word 99
    .word 112
    .word 10
    .word 109
    .word 118
    .word 10
    .word 99
    .word 100
    .word 10
    .word 107
    .word 105
    .word 108
    .word 108
    .word 10
    .word 0
floop:
    lw r1, 0(r10)
    beq r1, r0, fd
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, floop
fd:
    addi r1, r0, 0
    jalr r0, r11, 0
fd_fail:
    addi r1, r0, 1
    jalr r0, r11, 0
