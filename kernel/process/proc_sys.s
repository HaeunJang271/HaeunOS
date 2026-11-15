# System call handlers. PCB pointer in r8. Args in PCB saved regs.
.isa 4
.global sys_exit
.global sys_fault_exit
.global sys_write
.global sys_read
.global sys_open
.global sys_close
.global sys_fork
.global sys_exec
.global sys_wait
.global sys_sleep
.global sys_getpid
.global sys_yield
.global advance_epc
.global restore_ret
.global pick_next
.global wake_all
.global wake_wait

sys_exit:
    lw r1, 68(r8)
    sw r1, 16(r8)
    addi r1, r0, 5
    sw r1, 0(r8)
    jal r15, wake_wait
    jal r15, pick_next
    bne r1, r0, sy_ex_ok
    # No READY: expire SLEEP deadlines once, then retry (minimal idle assist).
    jal r15, wake_all
    jal r15, pick_next
    bne r1, r0, sy_ex_ok
    lw r1, 16(r8)
    halt
sy_ex_ok:
    jal r0, restore_ret

# User fatal trap: record cause/tval/epc/pid, ZOMBIE once, then schedule.
sys_fault_exit:
    lw r1, 0(r8)
    addi r2, r0, 5
    beq r1, r2, sf_sched
    csrrs r1, 3, r0
    sw r1, 16(r8)
    jal r2, p_fdiag
    .word 0x0000A450
p_fdiag:
    lw r2, 0(r2)
    sw r1, 0(r2)
    csrrs r1, 4, r0
    sw r1, 4(r2)
    lw r1, 4(r8)
    sw r1, 8(r2)
    lw r1, 12(r8)
    sw r1, 12(r2)
    addi r1, r0, 5
    sw r1, 0(r8)
    jal r15, wake_wait
sf_sched:
    jal r15, pick_next
    bne r1, r0, sf_ok
    jal r15, wake_all
    jal r15, pick_next
    bne r1, r0, sf_ok
    lw r1, 16(r8)
    halt
sf_ok:
    jal r0, restore_ret

sys_write:
    lw r2, 68(r8)
    lw r3, 72(r8)
    jal r15, fd_check
    bne r1, r0, wr_bad
    jal r2, p_uart
    .word 0x10002000
p_uart:
    lw r2, 0(r2)
    sw r3, 0(r2)
    sw r0, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
wr_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_read:
    lw r2, 68(r8)
    jal r15, fd_check
    bne r1, r0, rd_bad
    jal r2, p_u2
    .word 0x10002000
p_u2:
    lw r2, 0(r2)
    lw r1, 0(r2)
    # Empty DATA reads have bit31 set (negative as signed).
    cmplt r3, r1, r0
    bne r3, r0, rd_empty
    addi r2, r0, 255
    and r1, r1, r2
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
rd_empty:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
rd_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_open:
    lw r2, 68(r8)
    addi r1, r0, 1
    bne r2, r1, op_bad
    addi r3, r0, 0
op_scan:
    addi r1, r0, 4
    beq r3, r1, op_bad
    addi r4, r0, 44
    add r4, r4, r3
    add r4, r4, r3
    add r4, r4, r3
    add r4, r4, r3
    add r4, r8, r4
    lw r1, 0(r4)
    beq r1, r0, op_got
    addi r3, r3, 1
    jal r0, op_scan
op_got:
    addi r1, r0, 1
    sw r1, 0(r4)
    sw r3, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
op_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_close:
    lw r2, 68(r8)
    addi r1, r0, 4
    cmplt r3, r2, r1
    beq r3, r0, cl_bad
    cmplt r3, r2, r0
    bne r3, r0, cl_bad
    addi r4, r0, 44
    add r4, r4, r2
    add r4, r4, r2
    add r4, r4, r2
    add r4, r4, r2
    add r4, r8, r4
    lw r1, 0(r4)
    beq r1, r0, cl_bad
    sw r0, 0(r4)
    sw r0, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret
cl_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_getpid:
    lw r1, 4(r8)
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_yield:
    jal r15, advance_epc
    addi r1, r0, 1
    sw r1, 0(r8)
    jal r15, pick_next
    jal r0, restore_ret

sys_sleep:
    lw r2, 68(r8)
    cmplt r1, r2, r0
    bne r1, r0, sl_bad
    beq r2, r0, sl_zero
    jal r1, p_tm
    .word 0x10001000
p_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    add r1, r1, r2
    sw r1, 20(r8)
    addi r1, r0, 3
    sw r1, 0(r8)
    jal r15, advance_epc
    jal r15, pick_next
    jal r0, restore_ret
sl_zero:
    jal r15, advance_epc
    jal r0, restore_ret
sl_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

sys_exec:
    lw r2, 68(r8)
    addi r1, r0, 3
    and r1, r2, r1
    bne r1, r0, ex_bad
    sw r2, 12(r8)
    lw r1, 36(r8)
    sw r1, 116(r8)
    lw r1, 40(r8)
    sw r1, 120(r8)
    sw r0, 64(r8)
    jal r0, restore_ret
ex_bad:
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

# fd_check: r2=fd -> r1=0 ok, 1 bad. Console device value 1.
# Uses r5 as scratch so CALLERS may keep r3 (WRITE data byte).
fd_check:
    addi r1, r0, 4
    cmplt r5, r2, r1
    beq r5, r0, fd_bad
    cmplt r5, r2, r0
    bne r5, r0, fd_bad
    addi r4, r0, 44
    add r4, r4, r2
    add r4, r4, r2
    add r4, r4, r2
    add r4, r4, r2
    add r4, r8, r4
    lw r1, 0(r4)
    addi r5, r0, 1
    bne r1, r5, fd_bad
    addi r1, r0, 0
    jalr r0, r15, 0
fd_bad:
    addi r1, r0, 1
    jalr r0, r15, 0

wake_wait:
    addi r10, r15, 0
    lw r9, 4(r8)
    jal r1, p_pb
    .word 0x0000A000
p_pb:
    lw r7, 0(r1)
    addi r6, r0, 0
ww_loop:
    lw r1, 0(r7)
    addi r2, r0, 4
    bne r1, r2, ww_next
    lw r1, 24(r7)
    beq r1, r0, ww_wake
    beq r1, r9, ww_wake
    jal r0, ww_next
ww_wake:
    addi r1, r0, 1
    sw r1, 0(r7)
ww_next:
    addi r7, r7, 0x100
    addi r6, r6, 1
    addi r1, r0, 4
    cmplt r2, r6, r1
    bne r2, r0, ww_loop
    jalr r0, r10, 0
