# Firmware disk boot: multi-block HXF (1 LOAD) -> HBI -> entry.
# Profile: file_size<=0x8000, VA>=0x00100000, single segment, ISA 1..4.
.isa 4
.global _start
.global fail
.global read_block
_start:
    jal r1, p_blk
    .word 0x10003000
p_blk:
    lw r8, 0(r1)
    jal r1, p_buf
    .word 0x00008000
p_buf:
    lw r9, 0(r1)
    addi r1, r0, 0
    addi r2, r9, 0
    jal r15, read_block
    lw r1, 0(r9)
    jal r2, p_mag
    .word 0x31465848
p_mag:
    lw r2, 0(r2)
    bne r1, r2, fail
    lw r4, 28(r9)
    jal r1, p_max
    .word 0x00008000
p_max:
    lw r1, 0(r1)
    cmplt r2, r1, r4
    bne r2, r0, fail
    beq r4, r0, fail
    addi r5, r0, 0
    addi r1, r4, 0
count_loop:
    beq r1, r0, count_done
    addi r5, r5, 1
    addi r3, r0, 512
    cmplt r2, r1, r3
    bne r2, r0, count_last
    sub r1, r1, r3
    jal r0, count_loop
count_last:
    addi r1, r0, 0
    jal r0, count_loop
count_done:
    addi r6, r0, 1
read_more:
    cmplt r2, r6, r5
    beq r2, r0, image_ready
    addi r1, r6, 0
    addi r3, r0, 512
    addi r2, r9, 0
    addi r7, r6, 0
mul512:
    beq r7, r0, mul_done
    add r2, r2, r3
    addi r7, r7, -1
    jal r0, mul512
mul_done:
    jal r15, read_block
    addi r6, r6, 1
    jal r0, read_more
image_ready:
    jal r15, load_one
    jal r15, write_hbi
    jal r1, p_hbi
    .word 0x0feff000
p_hbi:
    lw r2, 0(r1)
    addi r3, r0, 32
    jal r1, p_stbase
    .word 0x0ff00000
p_stbase:
    lw r14, 0(r1)
    jal r1, p_sttop
    .word 0x10000000
p_sttop:
    lw r13, 0(r1)
    jalr r0, r7, 0

read_block:
    sw r2, 12(r8)
    sw r1, 8(r8)
    addi r1, r0, 1
    sw r1, 4(r8)
    lw r1, 0(r8)
    addi r2, r0, 2
    bne r1, r2, fail
    jalr r0, r15, 0

fail:
    jal r1, p_uart
    .word 0x10002000
p_uart:
    lw r1, 0(r1)
    addi r2, r0, 0x45
    sw r2, 0(r1)
    addi r2, r0, 0x0a
    sw r2, 0(r1)
    addi r1, r0, 0
    halt

# load_one: HXF at r9 -> r7=entry, r6=isa. Uses r10..r12.
load_one:
    addi r10, r15, 0
    lw r1, 4(r9)
    jal r2, p_hdr
    .word 0x00010040
p_hdr:
    lw r2, 0(r2)
    bne r1, r2, fail
    lw r6, 8(r9)
    beq r6, r0, fail
    addi r1, r0, 5
    cmplt r2, r6, r1
    beq r2, r0, fail
    lw r1, 12(r9)
    bne r1, r0, fail
    lw r1, 20(r9)
    addi r2, r0, 64
    bne r1, r2, fail
    lw r1, 24(r9)
    jal r2, p_segmeta
    .word 0x00200001
p_segmeta:
    lw r2, 0(r2)
    bne r1, r2, fail
    lw r7, 16(r9)
    addi r1, r7, 0
    addi r2, r0, 3
    and r1, r1, r2
    bne r1, r0, fail
    addi r11, r9, 64
    lw r1, 0(r11)
    addi r2, r0, 1
    bne r1, r2, fail
    lw r1, 4(r11)
    addi r2, r0, 4
    and r1, r1, r2
    beq r1, r0, fail
    lw r1, 28(r11)
    bne r1, r0, fail
    lw r1, 24(r11)
    addi r2, r0, 4
    cmplt r3, r1, r2
    bne r3, r0, fail
    lw r1, 16(r11)
    lw r2, 20(r11)
    cmplt r3, r2, r1
    bne r3, r0, fail
    addi r2, r0, 3
    and r3, r1, r2
    bne r3, r0, fail
    lw r2, 20(r11)
    sub r2, r2, r1
    addi r1, r0, 3
    and r2, r2, r1
    bne r2, r0, fail
    lw r12, 12(r11)
    jal r1, p_minva
    .word 0x00100000
p_minva:
    lw r1, 0(r1)
    cmplt r2, r12, r1
    bne r2, r0, fail
    lw r1, 8(r11)
    addi r2, r0, 96
    cmplt r3, r1, r2
    bne r3, r0, fail
    lw r2, 16(r11)
    add r3, r1, r2
    lw r2, 28(r9)
    cmplt r4, r2, r3
    bne r4, r0, fail
    add r3, r9, r1
    lw r4, 16(r11)
    addi r5, r12, 0
copy_loop:
    beq r4, r0, copy_done
    lw r1, 0(r3)
    sw r1, 0(r5)
    addi r3, r3, 4
    addi r5, r5, 4
    addi r4, r4, -4
    jal r0, copy_loop
copy_done:
    lw r1, 16(r11)
    lw r2, 20(r11)
    sub r2, r2, r1
zero_loop:
    beq r2, r0, zero_done
    sw r0, 0(r5)
    addi r5, r5, 4
    addi r2, r2, -4
    jal r0, zero_loop
zero_done:
    cmplt r1, r7, r12
    bne r1, r0, fail
    lw r1, 20(r11)
    add r1, r12, r1
    cmplt r2, r7, r1
    beq r2, r0, fail
    jalr r0, r10, 0

write_hbi:
    addi r10, r15, 0
    jal r1, p_hb
    .word 0x0feff000
p_hb:
    lw r1, 0(r1)
    jal r2, p_hm
    .word 0x31494248
p_hm:
    lw r2, 0(r2)
    sw r2, 0(r1)
    jal r2, p_hv
    .word 0x00200001
p_hv:
    lw r2, 0(r2)
    sw r2, 4(r1)
    sw r0, 8(r1)
    jal r2, p_ram
    .word 0x10000000
p_ram:
    lw r2, 0(r2)
    sw r2, 12(r1)
    jal r2, p_sb
    .word 0x0ff00000
p_sb:
    lw r2, 0(r2)
    sw r2, 16(r1)
    jal r2, p_st
    .word 0x10000000
p_st:
    lw r2, 0(r2)
    sw r2, 20(r1)
    sw r6, 24(r1)
    sw r0, 28(r1)
    jalr r0, r10, 0
