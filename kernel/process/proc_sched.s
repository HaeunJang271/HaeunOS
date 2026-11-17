# Scheduler core: demote, invoke policy, validate, switch. RR lives in proc_policy.s.
# SCHED_SLOTS is 3 (PCB index 0..2). Slot 3 has no address space.
# IDLE_FLAG at 0xA460 is 1 only after the user PCB has been saved.
# Deadlock: no READY/RUNNING and no SLEEP -> halt.
#   ZOMBIE current: r1 = exit_code. Otherwise r1 = 4 (WAIT deadlock).
.isa 4
.global pick_next
.global wake_all
.global arm_timer
.global set_curr
.global switch_as
.global cur_pcb
.global restore_ret
.global sched_slots
.global resume_or_idle
.global idle_enter
.global idle_trap
.global policy_select
.global telem_leave_run
.global telem_ready
.global telem_select
.global telem_wsleep
.global telem_inc_timer

# r1 = SCHED_SLOTS (3). The only literal of that limit.
sched_slots:
    addi r1, r0, 3
    jalr r0, r15, 0

# pick_next (core): demote RUNNING, policy_select, validate, commit.
# r1=1 and r8=selected PCB, or r1=0 and r8=current PCB.
pick_next:
    addi r11, r15, 0
    jal r15, cur_pcb
    lw r1, 0(r8)
    addi r2, r0, 2
    bne r1, r2, pn_start
    jal r15, telem_leave_run
    addi r1, r0, 1
    sw r1, 0(r8)
    jal r15, telem_ready
pn_start:
    jal r1, p_cu
    .word 0x0000A400
p_cu:
    lw r7, 0(r1)
    lw r6, 0(r7)
    addi r13, r6, 0
    jal r15, policy_select
    beq r1, r0, pn_none
    jal r15, sched_slots
    addi r4, r1, 0
    cmplt r2, r6, r4
    beq r2, r0, pn_none
    jal r1, p_pb
    .word 0x0000A000
p_pb:
    lw r8, 0(r1)
    addi r2, r0, 0
pn_mul:
    beq r2, r6, pn_have
    addi r8, r8, 0x100
    addi r2, r2, 1
    jal r0, pn_mul
pn_have:
    lw r1, 0(r8)
    addi r2, r0, 1
    beq r1, r2, pn_ok
    addi r2, r0, 2
    beq r1, r2, pn_ok
    jal r0, pn_none
pn_ok:
    jal r15, telem_select
    addi r1, r0, 2
    sw r1, 0(r8)
    jal r15, set_curr
    jal r15, switch_as
    addi r1, r0, 1
    jalr r0, r11, 0
pn_none:
    jal r15, cur_pcb
    addi r1, r0, 0
    jalr r0, r11, 0

# set_curr: PCB in r8 -> curr index
set_curr:
    addi r10, r15, 0
    jal r1, p_base
    .word 0x0000A000
p_base:
    lw r1, 0(r1)
    sub r2, r8, r1
    addi r3, r0, 0
sc_div:
    beq r2, r0, sc_wr
    addi r2, r2, -256
    addi r3, r3, 1
    jal r0, sc_div
sc_wr:
    jal r1, p_cux
    .word 0x0000A400
p_cux:
    lw r1, 0(r1)
    sw r3, 0(r1)
    jalr r0, r10, 0

switch_as:
    lw r1, 28(r8)
    csrrw r0, 6, r1
    sfence
    jalr r0, r15, 0

# Due when signed(now_lo - deadline) >= 0. Sleep length is < 2^31.
wake_all:
    addi r11, r15, 0
    jal r1, p_tmr
    .word 0x10001000
p_tmr:
    lw r1, 0(r1)
    lw r9, 0(r1)
    jal r15, sched_slots
    addi r7, r1, 0
    jal r1, p_pb2
    .word 0x0000A000
p_pb2:
    lw r8, 0(r1)
    addi r6, r0, 0
wk_loop:
    beq r6, r7, wk_done
    lw r1, 0(r8)
    addi r2, r0, 3
    bne r1, r2, wk_next
    lw r1, 20(r8)
    sub r2, r9, r1
    cmplt r2, r2, r0
    bne r2, r0, wk_next
    jal r15, telem_wsleep
    addi r1, r0, 1
    sw r1, 0(r8)
    sw r0, 20(r8)
wk_next:
    addi r8, r8, 0x100
    addi r6, r6, 1
    jal r0, wk_loop
wk_done:
    jal r15, cur_pcb
    jalr r0, r11, 0

# compare = timer_count + 96, both halves.
# 32 ticks are the user quantum. The rest cover this routine plus
# ack_timer and restore_ret, which retire before ERET. Arming only
# +32 makes the one-shot due inside the handler; ERET then takes the
# IRQ and do_timer undoes the context switch before user code runs.
arm_timer:
    addi r10, r15, 0
    jal r2, p_tm2
    .word 0x10001000
p_tm2:
    lw r2, 0(r2)
    lw r3, 0(r2)
    lw r4, 4(r2)
    addi r5, r3, 32
    addi r5, r5, 32
    addi r5, r5, 32
    jal r1, p_sign
    .word -2147483648
p_sign:
    lw r1, 0(r1)
    xor r6, r5, r1
    xor r7, r3, r1
    cmplt r6, r6, r7
    beq r6, r0, at_store
    addi r4, r4, 1
at_store:
    sw r4, 12(r2)
    sw r5, 8(r2)
    addi r3, r0, 1
    sw r3, 16(r2)
    jalr r0, r10, 0

# r1=1 if any schedulable PCB is SLEEP.
any_sleep:
    addi r11, r15, 0
    jal r15, sched_slots
    addi r7, r1, 0
    jal r1, as_base
    .word 0x0000A000
as_base:
    lw r8, 0(r1)
    addi r6, r0, 0
as_loop:
    beq r6, r7, as_no
    lw r1, 0(r8)
    addi r2, r0, 3
    beq r1, r2, as_yes
    addi r8, r8, 0x100
    addi r6, r6, 1
    jal r0, as_loop
as_yes:
    addi r1, r0, 1
    jalr r0, r11, 0
as_no:
    addi r1, r0, 0
    jalr r0, r11, 0

# Success: user eret. Failure with a sleeper: wait for the timer IRQ.
resume_or_idle:
    jal r15, pick_next
    beq r1, r0, idle_enter
    jal r0, restore_ret

idle_enter:
    jal r1, ie_fl
    .word 0x0000A460
ie_fl:
    lw r1, 0(r1)
    addi r2, r0, 1
    sw r2, 0(r1)
ie_loop:
    jal r15, wake_all
    jal r15, pick_next
    bne r1, r0, ie_go
    jal r15, any_sleep
    beq r1, r0, idle_dead
    jal r15, arm_timer
    csrrs r1, 0, r0
    addi r2, r0, 1
    or r1, r1, r2
    csrrw r0, 0, r1
ie_spin:
    jal r0, ie_spin
ie_go:
    jal r1, ie_fl2
    .word 0x0000A460
ie_fl2:
    lw r1, 0(r1)
    sw r0, 0(r1)
    addi r1, r0, 2
    csrrw r0, 0, r1
    jal r0, restore_ret

# Timer IRQ while idle_enter is spinning. Does not rewrite a saved user PCB.
idle_trap:
    csrrs r1, 3, r0
    addi r2, r0, 16
    bne r1, r2, it_kfault
    jal r15, telem_inc_timer
    jal r2, p_irq_i
    .word 0x10000000
p_irq_i:
    lw r2, 0(r2)
    sw r0, 12(r2)
    jal r15, wake_all
    jal r15, pick_next
    beq r1, r0, it_wait
    jal r1, it_fl
    .word 0x0000A460
it_fl:
    lw r1, 0(r1)
    sw r0, 0(r1)
    addi r1, r0, 2
    csrrw r0, 0, r1
    jal r0, restore_ret
it_kfault:
    addi r1, r0, 0x4B
    halt
it_wait:
    jal r15, any_sleep
    beq r1, r0, idle_dead
    jal r15, arm_timer
    eret

idle_dead:
    jal r15, cur_pcb
    lw r1, 0(r8)
    addi r2, r0, 5
    bne r1, r2, id_wait
    lw r1, 16(r8)
    halt
id_wait:
    addi r1, r0, 4
    halt
