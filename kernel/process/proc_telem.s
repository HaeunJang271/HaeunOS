# R1.2 observational telemetry. Helpers use link in r14 via jal r15 + addi r14.
# PCB in r8 unless noted. COUNT_LO via unsigned modular subtraction.
# R3.4: ready_seq globals 0xA46C/0xA470; wrap sticky 0xA478; PCB +172/+176.
# HOBJ symbol names must be <= 15 bytes.
.isa 4
.global telem_leave_run
.global telem_ready
.global telem_sleep
.global telem_block
.global telem_wsleep
.global telem_wwait
.global telem_select
.global telem_complete
.global telem_inc_timer
.global telem_child
.global telem_boot_init
.global sched_pol_init
.global ready_seq_take

# run_ticks += COUNT_LO - account_lo. Call before leaving RUNNING.
telem_leave_run:
    addi r14, r15, 0
    jal r1, tlr_tm
    .word 0x10001000
tlr_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    lw r2, 156(r8)
    sub r1, r1, r2
    lw r2, 128(r8)
    add r2, r2, r1
    sw r2, 128(r8)
    jalr r0, r14, 0

# Out r1=lo r3=hi. Clobbers r2,r9,r10. Return in r15 (do not touch r11).
ready_seq_take:
    jal r2, rst_lo
    .word 0x0000A46C
rst_lo:
    lw r2, 0(r2)
    lw r1, 0(r2)
    lw r3, 4(r2)
    addi r1, r1, 1
    bne r1, r0, rst_store
    addi r3, r3, 1
    bne r3, r0, rst_store
    jal r9, rst_w
    .word 0x0000A478
rst_w:
    lw r9, 0(r9)
    addi r10, r0, 1
    sw r10, 0(r9)
rst_store:
    sw r1, 0(r2)
    sw r3, 4(r2)
    jalr r0, r15, 0

telem_ready:
    addi r14, r15, 0
    jal r1, trd_tm
    .word 0x10001000
trd_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 160(r8)
    jal r15, ready_seq_take
    sw r1, 172(r8)
    sw r3, 176(r8)
    jalr r0, r14, 0

telem_sleep:
    addi r14, r15, 0
    jal r1, tsl_tm
    .word 0x10001000
tsl_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 164(r8)
    jalr r0, r14, 0

telem_block:
    addi r14, r15, 0
    jal r1, tbk_tm
    .word 0x10001000
tbk_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 168(r8)
    jalr r0, r14, 0

# SLEEP -> READY. PCB r8.
telem_wsleep:
    addi r14, r15, 0
    jal r1, tws_tm
    .word 0x10001000
tws_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    lw r2, 164(r8)
    sub r3, r1, r2
    lw r2, 136(r8)
    add r2, r2, r3
    sw r2, 136(r8)
    sw r1, 160(r8)
    jal r15, ready_seq_take
    sw r1, 172(r8)
    sw r3, 176(r8)
    jalr r0, r14, 0

# WAIT -> READY. PCB r7.
telem_wwait:
    addi r14, r15, 0
    jal r1, tww_tm
    .word 0x10001000
tww_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    lw r2, 168(r7)
    sub r3, r1, r2
    lw r2, 140(r7)
    add r2, r2, r3
    sw r2, 140(r7)
    sw r1, 160(r7)
    jal r15, ready_seq_take
    sw r1, 172(r7)
    sw r3, 176(r7)
    jalr r0, r14, 0

# r8=PCB, r13=old curr, r6=new index.
telem_select:
    addi r14, r15, 0
    lw r1, 0(r8)
    addi r2, r0, 1
    bne r1, r2, ts_disp
    jal r1, ts_tm
    .word 0x10001000
ts_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    lw r2, 160(r8)
    sub r1, r1, r2
    lw r2, 132(r8)
    add r2, r2, r1
    sw r2, 132(r8)
ts_disp:
    lw r1, 144(r8)
    addi r1, r1, 1
    sw r1, 144(r8)
    beq r6, r13, ts_acct
    jal r1, ts_cx
    .word 0x0000A468
ts_cx:
    lw r1, 0(r1)
    lw r2, 0(r1)
    addi r2, r2, 1
    sw r2, 0(r1)
ts_acct:
    jal r1, ts_tm2
    .word 0x10001000
ts_tm2:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 156(r8)
    jalr r0, r14, 0

telem_complete:
    addi r14, r15, 0
    lw r1, 152(r8)
    bne r1, r0, tc_done
    jal r1, tc_tm
    .word 0x10001000
tc_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 148(r8)
    addi r1, r0, 1
    sw r1, 152(r8)
tc_done:
    jalr r0, r14, 0

telem_inc_timer:
    addi r14, r15, 0
    jal r1, tit_p
    .word 0x0000A464
tit_p:
    lw r1, 0(r1)
    lw r2, 0(r1)
    addi r2, r2, 1
    sw r2, 0(r1)
    jalr r0, r14, 0

# Child PCB r7: zero telemetry + ready_seq, set ready_since and seq.
telem_child:
    addi r14, r15, 0
    sw r0, 128(r7)
    sw r0, 132(r7)
    sw r0, 136(r7)
    sw r0, 140(r7)
    sw r0, 144(r7)
    sw r0, 148(r7)
    sw r0, 152(r7)
    sw r0, 156(r7)
    sw r0, 160(r7)
    sw r0, 164(r7)
    sw r0, 168(r7)
    sw r0, 172(r7)
    sw r0, 176(r7)
    jal r1, tci_tm
    .word 0x10001000
tci_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 160(r7)
    jal r15, ready_seq_take
    sw r1, 172(r7)
    sw r3, 176(r7)
    jalr r0, r14, 0

telem_boot_init:
    addi r14, r15, 0
    jal r1, tbi_g0
    .word 0x0000A464
tbi_g0:
    lw r1, 0(r1)
    sw r0, 0(r1)
    jal r1, tbi_g1
    .word 0x0000A468
tbi_g1:
    lw r1, 0(r1)
    sw r0, 0(r1)
    jal r1, tbi_g2
    .word 0x0000A46C
tbi_g2:
    lw r1, 0(r1)
    sw r0, 0(r1)
    sw r0, 4(r1)
    jal r1, tbi_g3
    .word 0x0000A478
tbi_g3:
    lw r1, 0(r1)
    sw r0, 0(r1)
    # R3.0: copy build-time sched_pol_boot into 0xA474 (default RR=0).
    jal r15, sched_pol_init
    jal r8, tbi_pcb
    .word 0x0000A000
tbi_pcb:
    lw r8, 0(r8)
    jal r1, tbi_tm
    .word 0x10001000
tbi_tm:
    lw r1, 0(r1)
    lw r1, 0(r1)
    sw r1, 156(r8)
    sw r1, 160(r8)
    jal r15, ready_seq_take
    sw r1, 172(r8)
    sw r3, 176(r8)
    jalr r0, r14, 0
