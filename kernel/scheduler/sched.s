# Trap: syscalls + timer preemption (cause 16). Users page-aligned below.
.isa 4
.global trap_entry
.global user_a
.global user_b

trap_entry:
    jal r8, p_sc
    .word 0x00005110
p_sc:
    lw r8, 0(r8)
    sw r1, 0(r8)
    sw r9, 4(r8)
    sw r10, 8(r8)
    sw r11, 12(r8)
    jal r10, p_curr
    .word 0x00005100
p_curr:
    lw r10, 0(r10)
    lw r9, 0(r10)
    jal r8, p_t0
    .word 0x00005000
p_t0:
    lw r8, 0(r8)
    beq r9, r0, have_tcb
    addi r8, r8, 0x80
have_tcb:
    csrrs r11, 2, r0
    sw r11, 4(r8)
    sw r2, 12(r8)
    sw r3, 16(r8)
    sw r4, 20(r8)
    sw r5, 24(r8)
    sw r6, 28(r8)
    sw r7, 32(r8)
    sw r12, 52(r8)
    sw r13, 56(r8)
    sw r14, 60(r8)
    sw r15, 64(r8)
    jal r1, p_sc2
    .word 0x00005110
p_sc2:
    lw r1, 0(r1)
    lw r2, 0(r1)
    sw r2, 8(r8)
    lw r2, 4(r1)
    sw r2, 40(r8)
    lw r2, 8(r1)
    sw r2, 44(r8)
    lw r2, 12(r1)
    sw r2, 48(r8)
    sw r0, 36(r8)
    csrrs r1, 3, r0
    addi r2, r0, 16
    beq r1, r2, do_timer
    lw r1, 8(r8)
    beq r1, r0, do_exit
    addi r2, r0, 1
    beq r1, r2, do_putc
    addi r2, r0, 2
    beq r1, r2, do_yield
    addi r1, r0, -1
    sw r1, 8(r8)
    jal r0, advance_same

do_timer:
    jal r2, p_irqc
    .word 0x10000000
p_irqc:
    lw r2, 0(r2)
    sw r0, 12(r2)
    jal r15, pick_next
    bne r1, r0, install_timer
    jal r0, rearm_eret
install_timer:
    jal r1, p_t0b
    .word 0x00005000
p_t0b:
    lw r1, 0(r1)
    addi r2, r0, 0
    beq r8, r1, write_curr_t
    addi r2, r0, 1
write_curr_t:
    jal r1, p_currb
    .word 0x00005100
p_currb:
    lw r1, 0(r1)
    sw r2, 0(r1)
rearm_eret:
    jal r2, p_tmr
    .word 0x10001000
p_tmr:
    lw r2, 0(r2)
    lw r3, 0(r2)
    addi r3, r3, 32
    sw r3, 8(r2)
    addi r3, r0, 1
    sw r3, 16(r2)
    jal r0, restore_eret

do_putc:
    jal r2, p_uart
    .word 0x10002000
p_uart:
    lw r2, 0(r2)
    lw r3, 12(r8)
    sw r3, 0(r2)
    sw r0, 8(r8)
advance_same:
    lw r2, 4(r8)
    addi r2, r2, 4
    sw r2, 4(r8)
    jal r0, restore_eret

do_yield:
    lw r2, 4(r8)
    addi r2, r2, 4
    sw r2, 4(r8)
    jal r15, pick_next
    bne r1, r0, install_curr
    jal r0, restore_eret

do_exit:
    sw r0, 0(r8)
    lw r5, 12(r8)
    jal r15, pick_next
    bne r1, r0, install_curr
    addi r1, r5, 0
    halt

install_curr:
    jal r1, p_t0e
    .word 0x00005000
p_t0e:
    lw r1, 0(r1)
    addi r2, r0, 0
    beq r8, r1, write_curr
    addi r2, r0, 1
write_curr:
    jal r1, p_curre
    .word 0x00005100
p_curre:
    lw r1, 0(r1)
    sw r2, 0(r1)
restore_eret:
    lw r1, 4(r8)
    csrrw r0, 2, r1
    lw r1, 8(r8)
    lw r2, 12(r8)
    lw r3, 16(r8)
    lw r4, 20(r8)
    lw r5, 24(r8)
    lw r6, 28(r8)
    lw r7, 32(r8)
    lw r9, 40(r8)
    lw r10, 44(r8)
    lw r11, 48(r8)
    lw r12, 52(r8)
    lw r13, 56(r8)
    lw r14, 60(r8)
    lw r15, 64(r8)
    eret

pick_next:
    jal r2, p_currc
    .word 0x00005100
p_currc:
    lw r2, 0(r2)
    lw r3, 0(r2)
    addi r4, r0, 1
    xor r3, r3, r4
    jal r8, p_t0c
    .word 0x00005000
p_t0c:
    lw r8, 0(r8)
    beq r3, r0, pn_chk
    addi r8, r8, 0x80
pn_chk:
    lw r4, 0(r8)
    bne r4, r0, pn_ok
    jal r8, p_t0d
    .word 0x00005000
p_t0d:
    lw r8, 0(r8)
    beq r3, r0, pn_add
    jal r0, pn_chk2
pn_add:
    addi r8, r8, 0x80
pn_chk2:
    lw r4, 0(r8)
    beq r4, r0, pn_none
pn_ok:
    addi r1, r0, 1
    jalr r0, r15, 0
pn_none:
    addi r1, r0, 0
    jalr r0, r15, 0

.align 4096
user_a:
    addi r3, r0, 6
la:
    addi r1, r0, 1
    addi r2, r0, 0x41
    ecall
    addi r3, r3, -1
    bne r3, r0, la
    addi r1, r0, 0
    addi r2, r0, 0x31
    ecall
    halt

user_b:
    addi r3, r0, 6
lb:
    addi r1, r0, 1
    addi r2, r0, 0x42
    ecall
    addi r3, r3, -1
    bne r3, r0, lb
    addi r1, r0, 0
    addi r2, r0, 0x32
    ecall
    halt
