# Process trap: save full GPR context into PCB, dispatch timer/syscalls.
.isa 4
.global trap_entry
.global restore_ret
.global cur_pcb
.global advance_epc
.global do_timer
.global sys_exit
.global sys_fault_exit
.global sys_write
.global sys_read
.global sys_open
.global sys_close
.global sys_fork
.global sys_exec
.global sys_wait
.global sys_trywait
.global sys_sleep
.global sys_getpid
.global sys_yield
.global wake_all
.global pick_next
.global arm_timer

# Mem scratch on PCB page (mapped kernel RW at 0xA000): 0xA410..
trap_entry:
    sw r1, 0xA410(r0)
    sw r2, 0xA414(r0)
    sw r3, 0xA418(r0)
    sw r4, 0xA41c(r0)
    sw r5, 0xA420(r0)
    sw r6, 0xA424(r0)
    sw r7, 0xA428(r0)
    sw r8, 0xA42c(r0)
    sw r9, 0xA430(r0)
    sw r10, 0xA434(r0)
    sw r11, 0xA438(r0)
    sw r12, 0xA43c(r0)
    sw r13, 0xA440(r0)
    sw r14, 0xA444(r0)
    sw r15, 0xA448(r0)
    jal r15, cur_pcb
    csrrs r11, 2, r0
    sw r11, 12(r8)
    lw r1, 0xA440(r0)
    sw r1, 116(r8)
    lw r13, 32(r8)
    lw r1, 0xA410(r0)
    sw r1, 64(r8)
    lw r1, 0xA414(r0)
    sw r1, 68(r8)
    lw r1, 0xA418(r0)
    sw r1, 72(r8)
    lw r1, 0xA41c(r0)
    sw r1, 76(r8)
    lw r1, 0xA420(r0)
    sw r1, 80(r8)
    lw r1, 0xA424(r0)
    sw r1, 84(r8)
    lw r1, 0xA428(r0)
    sw r1, 88(r8)
    lw r1, 0xA42c(r0)
    sw r1, 92(r8)
    lw r1, 0xA430(r0)
    sw r1, 96(r8)
    lw r1, 0xA434(r0)
    sw r1, 100(r8)
    lw r1, 0xA438(r0)
    sw r1, 104(r8)
    lw r1, 0xA43c(r0)
    sw r1, 108(r8)
    lw r1, 0xA444(r0)
    sw r1, 120(r8)
    lw r1, 0xA448(r0)
    sw r1, 124(r8)
    csrrs r1, 3, r0
    addi r2, r0, 16
    beq r1, r2, do_timer
    addi r2, r0, 3
    beq r1, r2, do_syscall
    # Fatal synchronous fault: user -> kill process; kernel -> halt.
    csrrs r3, 0, r0
    addi r2, r0, 4
    and r3, r3, r2
    bne r3, r0, kernel_fault
    jal r0, sys_fault_exit

do_syscall:
    lw r1, 64(r8)
    beq r1, r0, sys_exit
    addi r2, r0, 1
    beq r1, r2, sys_write
    addi r2, r0, 2
    beq r1, r2, sys_read
    addi r2, r0, 3
    beq r1, r2, sys_open
    addi r2, r0, 4
    beq r1, r2, sys_close
    addi r2, r0, 5
    beq r1, r2, sys_fork
    addi r2, r0, 6
    beq r1, r2, sys_exec
    addi r2, r0, 7
    beq r1, r2, sys_wait
    addi r2, r0, 8
    beq r1, r2, sys_sleep
    addi r2, r0, 9
    beq r1, r2, sys_getpid
    addi r2, r0, 10
    beq r1, r2, sys_yield
    addi r2, r0, 52
    beq r1, r2, sys_trywait
    addi r1, r0, -1
    sw r1, 64(r8)
    jal r15, advance_epc
    jal r0, restore_ret

kernel_fault:
    addi r1, r0, 0x4B
    halt

do_timer:
    jal r2, p_irqc
    .word 0x10000000
p_irqc:
    lw r2, 0(r2)
    sw r0, 12(r2)
    jal r15, wake_all
    jal r15, pick_next
    jal r15, arm_timer
    jal r0, restore_ret

cur_pcb:
    addi r10, r15, 0
    jal r1, p_cu
    .word 0x0000A400
p_cu:
    lw r1, 0(r1)
    lw r9, 0(r1)
    jal r1, p_pb
    .word 0x0000A000
p_pb:
    lw r8, 0(r1)
    addi r2, r0, 0
cu_loop:
    beq r2, r9, cu_done
    addi r8, r8, 0x100
    addi r2, r2, 1
    jal r0, cu_loop
cu_done:
    jalr r0, r10, 0

advance_epc:
    lw r2, 12(r8)
    addi r2, r2, 4
    sw r2, 12(r8)
    jalr r0, r15, 0

restore_ret:
    lw r1, 12(r8)
    csrrw r0, 2, r1
    lw r1, 64(r8)
    lw r2, 68(r8)
    lw r3, 72(r8)
    lw r4, 76(r8)
    lw r5, 80(r8)
    lw r6, 84(r8)
    lw r7, 88(r8)
    lw r9, 96(r8)
    lw r10, 100(r8)
    lw r11, 104(r8)
    lw r12, 108(r8)
    lw r13, 116(r8)
    lw r14, 120(r8)
    lw r15, 124(r8)
    lw r8, 92(r8)
    eret
