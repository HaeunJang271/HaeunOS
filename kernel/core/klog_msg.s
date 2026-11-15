# Fixed kernel log strings via jal-link address of trailing .word data.
.isa 4
.global klog_banner
.global klog_ready
.global klog_panic

klog_banner:
    addi r8, r15, 0
    jal r1, bu
    .word 0x10002000
bu:
    lw r9, 0(r1)
    jal r10, bloop
    .word 91
    .word 72
    .word 65
    .word 69
    .word 85
    .word 78
    .word 45
    .word 79
    .word 83
    .word 93
    .word 32
    .word 107
    .word 101
    .word 114
    .word 110
    .word 101
    .word 108
    .word 32
    .word 105
    .word 110
    .word 105
    .word 116
    .word 10
    .word 0
bloop:
    lw r1, 0(r10)
    beq r1, r0, bdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, bloop
bdone:
    addi r1, r0, 0
    jalr r0, r8, 0

klog_ready:
    addi r8, r15, 0
    jal r1, ru
    .word 0x10002000
ru:
    lw r9, 0(r1)
    jal r10, rloop
    .word 107
    .word 101
    .word 114
    .word 110
    .word 101
    .word 108
    .word 32
    .word 114
    .word 101
    .word 97
    .word 100
    .word 121
    .word 10
    .word 0
rloop:
    lw r1, 0(r10)
    beq r1, r0, rdone
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, rloop
rdone:
    addi r1, r0, 0
    jalr r0, r8, 0

klog_panic:
    addi r8, r15, 0
    jal r1, pu
    .word 0x10002000
pu:
    lw r9, 0(r1)
    jal r10, ploop
    .word 112
    .word 97
    .word 110
    .word 105
    .word 99
    .word 32
    .word 0
ploop:
    lw r1, 0(r10)
    beq r1, r0, pdig
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, ploop
pdig:
    addi r1, r2, 48
    sw r1, 0(r9)
    addi r1, r0, 10
    sw r1, 0(r9)
    addi r1, r0, 0
    jalr r0, r8, 0
