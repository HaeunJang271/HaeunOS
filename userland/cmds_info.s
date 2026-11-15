# Informational shell commands (UART banners).
.isa 4
.global cmd_help
.global cmd_version
.global cmd_uname
.global cmd_cpuinfo
.global cmd_meminfo
.global cmd_clear
.global cmd_pwd
.global cmd_ps
.global cmd_df
.global cmd_mount
.global vfs_mount

cmd_help:
    addi r8, r15, 0
    jal r1, u
    .word 0x10002000
u:
    lw r9, 0(r1)
    jal r10, hloop
    .word 104
    .word 101
    .word 108
    .word 112
    .word 10
    .word 0
hloop:
    lw r1, 0(r10)
    beq r1, r0, hdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, hloop
hdone:
    jalr r0, r8, 0

cmd_version:
    addi r8, r15, 0
    jal r1, u2
    .word 0x10002000
u2:
    lw r9, 0(r1)
    jal r10, vloop
    .word 72
    .word 65
    .word 69
    .word 85
    .word 78
    .word 45
    .word 79
    .word 83
    .word 32
    .word 48
    .word 46
    .word 49
    .word 10
    .word 0
vloop:
    lw r1, 0(r10)
    beq r1, r0, vdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, vloop
vdone:
    jalr r0, r8, 0

cmd_uname:
    addi r8, r15, 0
    jal r1, u3
    .word 0x10002000
u3:
    lw r9, 0(r1)
    jal r10, uloop
    .word 72
    .word 65
    .word 69
    .word 85
    .word 78
    .word 45
    .word 51
    .word 50
    .word 10
    .word 0
uloop:
    lw r1, 0(r10)
    beq r1, r0, udone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, uloop
udone:
    jalr r0, r8, 0

cmd_cpuinfo:
    addi r8, r15, 0
    jal r1, u4
    .word 0x10002000
u4:
    lw r9, 0(r1)
    jal r10, cloop
    .word 72
    .word 65
    .word 69
    .word 85
    .word 78
    .word 45
    .word 51
    .word 50
    .word 32
    .word 82
    .word 73
    .word 83
    .word 67
    .word 10
    .word 0
cloop:
    lw r1, 0(r10)
    beq r1, r0, cdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, cloop
cdone:
    jalr r0, r8, 0

cmd_meminfo:
    addi r8, r15, 0
    jal r1, u5
    .word 0x10002000
u5:
    lw r9, 0(r1)
    jal r10, mloop
    .word 50
    .word 53
    .word 54
    .word 77
    .word 105
    .word 66
    .word 10
    .word 0
mloop:
    lw r1, 0(r10)
    beq r1, r0, mdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, mloop
mdone:
    jalr r0, r8, 0

cmd_clear:
    addi r8, r15, 0
    addi r2, r0, 12
    jal r1, u6
    .word 0x10002000
u6:
    lw r1, 0(r1)
    sw r2, 0(r1)
    jalr r0, r8, 0

cmd_pwd:
    addi r8, r15, 0
    jal r1, u7
    .word 0x10002000
u7:
    lw r9, 0(r1)
    addi r1, r0, 47
    sw r1, 0(r9)
    addi r1, r0, 10
    sw r1, 0(r9)
    jalr r0, r8, 0

cmd_ps:
    addi r8, r15, 0
    jal r1, u8
    .word 0x10002000
u8:
    lw r9, 0(r1)
    jal r10, ploop
    .word 112
    .word 105
    .word 100
    .word 32
    .word 49
    .word 10
    .word 0
ploop:
    lw r1, 0(r10)
    beq r1, r0, pdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, ploop
pdone:
    jalr r0, r8, 0

cmd_df:
    addi r8, r15, 0
    jal r1, u9
    .word 0x10002000
u9:
    lw r9, 0(r1)
    jal r10, dloop
    .word 104
    .word 102
    .word 115
    .word 10
    .word 0
dloop:
    lw r1, 0(r10)
    beq r1, r0, ddone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, dloop
ddone:
    jalr r0, r8, 0

cmd_mount:
    addi r8, r15, 0
    jal r15, vfs_mount
    jalr r0, r8, 0
