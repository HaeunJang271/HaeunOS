# M20.9 GUI launcher: sequential calc/edit/filemgr + NOSPC + optional service.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    addi r1, r0, 18
    addi r2, r0, 2
    addi r3, r0, 2
    addi r4, r0, 56
    addi r5, r0, 26
    jal r6, wbase
    .word 0x0001C000
wbase:
    lw r6, 0(r6)
    ecall
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, fail

    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, bgc
    .word 0x00181820
bgc:
    lw r3, 0(r3)
    ecall
    jal r15, paint_list

    addi r1, r0, 24
    ecall
    addi r2, r0, 3
    cmplt r3, r1, r2
    bne r3, r0, fail
    addi r14, r1, 0

    # NOSPC: fill both AS slots then third spawn must fail
    addi r1, r0, 26
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r11, r1, 0
    addi r1, r0, 26
    addi r2, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r12, r1, 0
    addi r1, r0, 26
    addi r2, r0, 2
    ecall
    cmplt r3, r1, r0
    beq r3, r0, fail
    addi r1, r0, 40
    addi r2, r0, 1
    addi r3, r11, 0
    ecall
    addi r1, r0, 40
    addi r2, r0, 1
    addi r3, r12, 0
    ecall
    jal r15, yield_n
    addi r1, r0, 7
    ecall
    jal r15, yield_n
    addi r1, r0, 7
    ecall

    # Sequential: spawn → yield (create win) → CLOSE → yield → WAIT
    addi r1, r0, 26
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_n
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_n
    addi r1, r0, 7
    ecall

    addi r1, r0, 26
    addi r2, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_n
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_n
    addi r1, r0, 7
    ecall

    addi r2, r0, 5
    cmplt r3, r14, r2
    bne r3, r0, skip_fm
    addi r1, r0, 26
    addi r2, r14, -1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    sw r1, -20(r13)
    jal r15, yield_n
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -20(r13)
    ecall
    jal r15, yield_n
    addi r1, r0, 7
    ecall
skip_fm:

    # bad spawn index
    addi r1, r0, 26
    addi r2, r0, 99
    ecall
    cmplt r3, r1, r0
    beq r3, r0, fail

    # optional service idle (id 0)
    addi r1, r0, 41
    ecall
    beq r1, r0, no_svc
    addi r1, r0, 43
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 45
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r4, r0, 2
    bne r2, r4, fail
    addi r1, r0, 44
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    jal r15, yield_n
    addi r1, r0, 7
    ecall
no_svc:
    jal r15, paint_ok
    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

paint_list:
    addi r12, r15, 0
    addi r1, r0, 28
    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r0, 2
    jal r5, fg
    .word 0x00E0E0E0
fg:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, t_apps
    .word 0x73707041
    .word 0x00000000
t_apps:
    ecall
    addi r1, r0, 24
    ecall
    addi r11, r1, 0
    addi r10, r0, 0
    addi r14, r0, 12
pl_loop:
    cmplt r3, r10, r11
    beq r3, r0, pl_done
    addi r1, r0, 25
    addi r2, r10, 0
    addi r3, r13, -32
    addi r4, r0, 15
    ecall
    addi r1, r0, 28
    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r14, 0
    jal r5, fg2
    .word 0x00C0FFC0
fg2:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -32
    ecall
    addi r10, r10, 1
    addi r14, r14, 8
    jal r0, pl_loop
pl_done:
    jalr r0, r12, 0

paint_ok:
    addi r12, r15, 0
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, okc
    .word 0x00004020
okc:
    lw r3, 0(r3)
    ecall
    addi r1, r0, 28
    addi r2, r8, 0
    addi r3, r0, 8
    addi r4, r0, 10
    jal r5, fg4
    .word 0x00FFFFFF
fg4:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, t_ok
    .word 0x00004B4F
t_ok:
    ecall
    jalr r0, r12, 0

yield_n:
    addi r10, r15, 0
    addi r9, r0, 128
yn:
    addi r1, r0, 10
    ecall
    addi r9, r9, -1
    bne r9, r0, yn
    jalr r0, r10, 0

fail:
    addi r1, r0, 0
    addi r2, r0, 0xe5
    ecall
    halt
