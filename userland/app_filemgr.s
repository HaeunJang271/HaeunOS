# M20.7 File Manager smoke: readdir, create/write/read, stat, unlink, redraw.
.isa 4
.global user_init

user_init:
    jal r15, app_getpid
    sw r1, -4(r13)

    # File manager on 320x180.
    addi r2, r0, 70
    addi r3, r0, 36
    addi r4, r0, 180
    addi r5, r0, 100
    jal r6, wbase
    .word 0x00090000
wbase:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, fail

    addi r2, r8, 0
    jal r3, bgc
    .word 0x00102018
bgc:
    lw r3, 0(r3)
    jal r15, app_win_fill

    # Title
    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r0, 2
    jal r5, fg
    .word 0x00E8E8E8
fg:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, t_fm
    .word 0x656C6946
    .word 0x00000000
t_fm:
    jal r15, app_draw_text

    # Create note (OPEN create)
    jal r2, p_note
    .word 0x65746F6E
    .word 0x00000000
p_note:
    addi r3, r0, 1
    jal r15, app_open
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r9, r1, 0
    addi r2, r9, 0
    addi r3, r0, 72
    jal r15, app_write
    addi r2, r9, 0
    addi r3, r0, 73
    jal r15, app_write
    addi r2, r9, 0
    jal r15, app_close

    # Seek+read back via reopen
    jal r2, p_note2
    .word 0x65746F6E
    .word 0x00000000
p_note2:
    addi r3, r0, 0
    jal r15, app_open
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r9, r1, 0
    addi r2, r9, 0
    jal r15, app_read
    addi r4, r0, 72
    bne r1, r4, fail
    addi r2, r9, 0
    jal r15, app_read
    addi r4, r0, 73
    bne r1, r4, fail
    addi r2, r9, 0
    jal r15, app_close

    # STAT note
    jal r2, p_note3
    .word 0x65746F6E
    .word 0x00000000
p_note3:
    addi r1, r0, 49
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r4, r0, 2
    bne r2, r4, fail

    # READDIR into stack buf
    addi r2, r13, -96
    addi r3, r0, 64
    addi r1, r0, 47
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail

    # Draw first dir entry bytes as text
    addi r2, r8, 0
    addi r3, r0, 2
    addi r4, r0, 12
    jal r5, fg2
    .word 0x00C0FFC0
fg2:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -96
    jal r15, app_draw_text

    # UNLINK note then recreate
    jal r2, p_note4
    .word 0x65746F6E
    .word 0x00000000
p_note4:
    addi r1, r0, 48
    ecall
    cmplt r3, r1, r0
    bne r3, r0, fail

    jal r2, p_note6
    .word 0x65746F6E
    .word 0x00000000
p_note6:
    addi r3, r0, 1
    jal r15, app_open
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r9, r1, 0
    addi r2, r9, 0
    addi r3, r0, 79
    jal r15, app_write
    addi r2, r9, 0
    addi r3, r0, 75
    jal r15, app_write
    addi r2, r9, 0
    jal r15, app_close

    jal r15, app_compose
fm_loop:
    jal r15, app_event_poll
    beq r1, r0, fm_idle
    addi r10, r0, 8
    beq r1, r10, fm_bye
    addi r10, r0, 1
    bne r1, r10, fm_loop
    addi r10, r0, 0x01
    beq r3, r10, fm_bye
    jal r0, fm_loop
fm_idle:
    addi r2, r8, 0
    jal r15, app_win_query
    bne r1, r0, fm_bye
    jal r15, app_yield
    jal r0, fm_loop
fm_bye:
    addi r2, r8, 0
    jal r15, app_win_query
    bne r1, r0, fm_exit
    addi r2, r8, 0
    jal r15, app_win_destroy
fm_exit:
    addi r2, r0, 1
    jal r15, app_exit

fail:
    addi r2, r0, 0xe7
    jal r15, app_exit
