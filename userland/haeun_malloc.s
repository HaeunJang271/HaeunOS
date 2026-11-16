# M25.6 fixed-arena first-fit (VA 0x109000, 4 KiB).
# Nested calloc/realloc save link on ustack (r13).
.isa 4
.global heap_init
.global malloc
.global free
.global calloc
.global realloc

heap_init:
    jal r2, hi_b
    .word 0x00109000
hi_b:
    lw r2, 0(r2)
    addi r1, r0, 4096
    sw r1, 0(r2)
    sw r0, 4(r2)
    jalr r0, r15, 0

malloc:
    addi r10, r15, 0
    beq r1, r0, m_fail
    addi r2, r1, 7
    addi r3, r0, -8
    and r2, r2, r3
    addi r2, r2, 8
    addi r3, r0, 16
    cmplt r4, r2, r3
    beq r4, r0, m_need
    addi r2, r0, 16
m_need:
    addi r3, r0, 4096
    cmplt r4, r3, r2
    bne r4, r0, m_fail
    jal r5, m_base
    .word 0x00109000
m_base:
    lw r5, 0(r5)
m_scan:
    jal r1, m_lim
    .word 0x0010A000
m_lim:
    lw r1, 0(r1)
    cmplt r6, r5, r1
    beq r6, r0, m_fail
    lw r6, 0(r5)
    addi r7, r0, 1
    and r7, r6, r7
    bne r7, r0, m_adv
    cmplt r7, r6, r2
    bne r7, r0, m_adv
    sub r7, r6, r2
    addi r9, r0, 16
    cmplt r9, r7, r9
    bne r9, r0, m_use
    add r9, r5, r2
    sw r7, 0(r9)
    sw r0, 4(r9)
    addi r6, r2, 0
m_use:
    addi r7, r0, 1
    or r6, r6, r7
    sw r6, 0(r5)
    sw r0, 4(r5)
    addi r1, r5, 8
    jalr r0, r10, 0
m_adv:
    lw r6, 0(r5)
    addi r7, r0, -2
    and r6, r6, r7
    add r5, r5, r6
    jal r0, m_scan
m_fail:
    addi r1, r0, 0
    jalr r0, r10, 0

free:
    addi r10, r15, 0
    beq r1, r0, f_ret
    addi r2, r1, -8
    jal r3, f_lo
    .word 0x00109000
f_lo:
    lw r3, 0(r3)
    cmplt r4, r2, r3
    bne r4, r0, f_ret
    jal r3, f_hi
    .word 0x0010A000
f_hi:
    lw r3, 0(r3)
    cmplt r4, r2, r3
    beq r4, r0, f_ret
    lw r3, 0(r2)
    addi r4, r0, 1
    and r4, r3, r4
    beq r4, r0, f_ret
    addi r4, r0, -2
    and r3, r3, r4
    sw r3, 0(r2)
    sw r0, 4(r2)
f_coal:
    lw r3, 0(r2)
    add r4, r2, r3
    jal r5, f_end
    .word 0x0010A000
f_end:
    lw r5, 0(r5)
    cmplt r6, r4, r5
    beq r6, r0, f_ret
    lw r6, 0(r4)
    addi r7, r0, 1
    and r7, r6, r7
    bne r7, r0, f_ret
    add r3, r3, r6
    sw r3, 0(r2)
    jal r0, f_coal
f_ret:
    jalr r0, r10, 0

calloc:
    addi r13, r13, -4
    sw r15, 0(r13)
    addi r3, r0, 0
    addi r4, r1, 0
    addi r5, r2, 0
    beq r5, r0, ca_sz0
ca_acc:
    beq r4, r0, ca_tot
    add r6, r3, r5
    cmplt r7, r6, r3
    bne r7, r0, ca_fail
    addi r3, r6, 0
    addi r4, r4, -1
    jal r0, ca_acc
ca_tot:
    addi r1, r3, 0
    jal r15, malloc
    beq r1, r0, ca_done
    addi r2, r1, 0
    addi r3, r2, -8
    lw r3, 0(r3)
    addi r4, r0, -2
    and r3, r3, r4
    addi r3, r3, -8
    addi r4, r0, 0
ca_z:
    beq r4, r3, ca_done
    add r5, r2, r4
    sw r0, 0(r5)
    addi r4, r4, 4
    jal r0, ca_z
ca_done:
    lw r15, 0(r13)
    addi r13, r13, 4
    jalr r0, r15, 0
ca_sz0:
    addi r1, r0, 0
    jal r15, malloc
    jal r0, ca_done
ca_fail:
    addi r1, r0, 0
    jal r0, ca_done

realloc:
    # stack: [0]=ret [4]=old_ptr [8]=new_ptr [12]=nsize
    addi r13, r13, -16
    sw r15, 0(r13)
    sw r1, 4(r13)
    sw r2, 12(r13)
    beq r1, r0, ra_new
    beq r2, r0, ra_free
    addi r1, r2, 0
    jal r15, malloc
    beq r1, r0, ra_done
    sw r1, 8(r13)
    lw r1, 4(r13)
    addi r6, r1, -8
    lw r6, 0(r6)
    addi r7, r0, -2
    and r6, r6, r7
    addi r6, r6, -8
    lw r3, 8(r13)
    addi r3, r3, -8
    lw r3, 0(r3)
    and r3, r3, r7
    addi r3, r3, -8
    cmplt r4, r3, r6
    beq r4, r0, ra_use
    addi r6, r3, 0
ra_use:
    addi r4, r0, 0
ra_cp:
    beq r4, r6, ra_old
    lw r2, 4(r13)
    add r2, r2, r4
    lw r2, 0(r2)
    lw r3, 8(r13)
    add r3, r3, r4
    sw r2, 0(r3)
    addi r4, r4, 4
    jal r0, ra_cp
ra_old:
    lw r1, 4(r13)
    jal r15, free
    lw r1, 8(r13)
    jal r0, ra_done
ra_new:
    lw r1, 12(r13)
    jal r15, malloc
    jal r0, ra_done
ra_free:
    lw r1, 4(r13)
    jal r15, free
    addi r1, r0, 0
ra_done:
    lw r15, 0(r13)
    addi r13, r13, 16
    jalr r0, r15, 0
