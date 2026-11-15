# Round-robin pick, sleep wakeups, timer re-arm, address-space switch.
.isa 4
.global pick_next
.global wake_all
.global arm_timer
.global set_curr
.global switch_as
.global cur_pcb

# pick_next: scan READY from next index; install into r8 / curr. r1=1 ok, 0 none.
pick_next:
    addi r11, r15, 0
    jal r15, cur_pcb
    lw r1, 0(r8)
    addi r2, r0, 2
    bne r1, r2, pn_start
    addi r1, r0, 1
    sw r1, 0(r8)
pn_start:
    jal r1, p_cu
    .word 0x0000A400
p_cu:
    lw r7, 0(r1)
    lw r6, 0(r7)
    addi r5, r0, 0
pn_scan:
    addi r6, r6, 1
    addi r1, r0, 4
    cmplt r2, r6, r1
    bne r2, r0, pn_okidx
    addi r6, r0, 0
pn_okidx:
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
    beq r1, r2, pn_found
    addi r2, r0, 2
    beq r1, r2, pn_found
    addi r5, r5, 1
    addi r1, r0, 4
    beq r5, r1, pn_none
    jal r0, pn_scan
pn_found:
    addi r1, r0, 2
    sw r1, 0(r8)
    jal r15, set_curr
    jal r15, switch_as
    addi r1, r0, 1
    jalr r0, r11, 0
pn_none:
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

wake_all:
    addi r11, r15, 0
    jal r1, p_tmr
    .word 0x10001000
p_tmr:
    lw r1, 0(r1)
    lw r9, 0(r1)
    jal r1, p_pb2
    .word 0x0000A000
p_pb2:
    lw r8, 0(r1)
    addi r6, r0, 0
wk_loop:
    lw r1, 0(r8)
    addi r2, r0, 3
    bne r1, r2, wk_next
    lw r1, 20(r8)
    cmplt r2, r9, r1
    bne r2, r0, wk_next
    addi r1, r0, 1
    sw r1, 0(r8)
    sw r0, 20(r8)
wk_next:
    addi r8, r8, 0x100
    addi r6, r6, 1
    addi r1, r0, 4
    cmplt r2, r6, r1
    bne r2, r0, wk_loop
    jal r0, wk_done
wk_done:
    jal r15, cur_pcb
    jalr r0, r11, 0

arm_timer:
    jal r2, p_tm2
    .word 0x10001000
p_tm2:
    lw r2, 0(r2)
    lw r3, 0(r2)
    addi r3, r3, 32
    sw r3, 8(r2)
    addi r3, r0, 1
    sw r3, 16(r2)
    jalr r0, r15, 0
