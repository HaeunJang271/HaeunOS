# M22.1.3+ HAEUN desktop shell (PCB0) on 320x180 FB.
# Stack: -8 win, -92 busy, -96 menu, -100 throttle. Keys 1-4 Space Esc.
.isa 4
.global user_init
.global shell_paint
.global shell_compose

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)
    # Fullscreen shell matching host FB 320x180.
    addi r1, r0, 18
    addi r2, r0, 0
    addi r3, r0, 0
    addi r4, r0, 320
    addi r5, r0, 180
    jal r6, wbase
    .word 0x00020000
wbase:
    lw r6, 0(r6)
    ecall
    cmplt r3, r1, r0
    bne r3, r0, die
    sw r1, -8(r13)
    sw r0, -92(r13)
    sw r0, -96(r13)
    sw r0, -100(r13)
    jal r15, shell_paint
    jal r15, shell_compose

loop:
    addi r1, r0, 27
    ecall
    beq r1, r0, idle
    addi r10, r0, 1
    beq r1, r10, on_key
    addi r10, r0, 4
    beq r1, r10, on_mouse
    jal r0, loop

on_key:
    addi r12, r3, 0
    addi r10, r0, 0x01
    beq r12, r10, shutdown
    addi r10, r0, 0x39
    beq r12, r10, tog_menu
    lw r1, -92(r13)
    beq r1, r0, try_spawn
    jal r0, after_ui
try_spawn:
    addi r10, r0, 0x02
    beq r12, r10, sp_calc
    addi r10, r0, 0x03
    beq r12, r10, sp_edit
    addi r10, r0, 0x04
    beq r12, r10, sp_fm
    addi r10, r0, 0x05
    beq r12, r10, sp_nospc
    jal r0, after_ui

tog_menu:
    lw r1, -96(r13)
    beq r1, r0, menu_on
    sw r0, -96(r13)
    jal r0, after_ui
menu_on:
    addi r1, r0, 1
    sw r1, -96(r13)
    jal r0, after_ui

sp_calc:
    addi r2, r0, 0
    jal r0, do_spawn
sp_edit:
    addi r2, r0, 1
    jal r0, do_spawn
sp_fm:
    # Builtin registry: 0=calc 1=edit 2=files (not count-1 / filemgr).
    addi r2, r0, 2
    jal r0, do_spawn

do_spawn:
    sw r0, -96(r13)
    addi r1, r0, 26
    ecall
    cmplt r3, r1, r0
    bne r3, r0, spawn_fail
    # Apps persist; brief busy debounce then return to event loop.
    addi r1, r0, 1
    sw r1, -92(r13)
    jal r15, shell_paint
    jal r15, shell_compose
    addi r1, r0, 10
    ecall
    addi r1, r0, 10
    ecall
    sw r0, -92(r13)
    jal r0, loop

spawn_fail:
    jal r15, shell_paint
    jal r15, shell_compose
    jal r0, loop

sp_nospc:
    addi r1, r0, 26
    addi r2, r0, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, spawn_fail
    sw r1, -84(r13)
    addi r1, r0, 26
    addi r2, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, spawn_fail
    sw r1, -88(r13)
    addi r1, r0, 26
    addi r2, r0, 2
    ecall
    cmplt r3, r1, r0
    beq r3, r0, spawn_fail
    # Reap fillers so later launches can use the AS slots.
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -84(r13)
    ecall
    addi r1, r0, 40
    addi r2, r0, 1
    lw r3, -88(r13)
    ecall
    jal r15, yield_burst
    addi r1, r0, 7
    ecall
    jal r15, yield_burst
    addi r1, r0, 7
    ecall
    jal r15, shell_paint
    jal r15, shell_compose
    jal r0, loop

on_mouse:
    addi r14, r3, 0
    addi r9, r4, 0
    # Tile0 Calculator (16,88)-(112,148)
    addi r1, r0, 16
    cmplt r2, r14, r1
    bne r2, r0, ms_bar
    addi r1, r0, 112
    cmplt r2, r14, r1
    beq r2, r0, ms_e
    addi r1, r0, 88
    cmplt r2, r9, r1
    bne r2, r0, ms_bar
    addi r1, r0, 148
    cmplt r2, r9, r1
    beq r2, r0, ms_e
    lw r1, -92(r13)
    beq r1, r0, sp_calc
    jal r0, after_ui
ms_e:
    addi r1, r0, 112
    cmplt r2, r14, r1
    bne r2, r0, ms_f
    addi r1, r0, 208
    cmplt r2, r14, r1
    beq r2, r0, ms_f
    addi r1, r0, 88
    cmplt r2, r9, r1
    bne r2, r0, ms_f
    addi r1, r0, 148
    cmplt r2, r9, r1
    beq r2, r0, ms_f
    lw r1, -92(r13)
    beq r1, r0, sp_edit
    jal r0, after_ui
ms_f:
    addi r1, r0, 208
    cmplt r2, r14, r1
    bne r2, r0, ms_bar
    addi r1, r0, 304
    cmplt r2, r14, r1
    beq r2, r0, ms_bar
    addi r1, r0, 88
    cmplt r2, r9, r1
    bne r2, r0, ms_bar
    addi r1, r0, 148
    cmplt r2, r9, r1
    beq r2, r0, ms_bar
    lw r1, -92(r13)
    beq r1, r0, sp_fm
    jal r0, after_ui
ms_bar:
    addi r1, r0, 0
    cmplt r2, r14, r1
    bne r2, r0, ms_raise
    addi r1, r0, 64
    cmplt r2, r14, r1
    beq r2, r0, ms_raise
    addi r1, r0, 164
    cmplt r2, r9, r1
    bne r2, r0, ms_raise
    jal r0, tog_menu
ms_raise:
    addi r1, r0, 22
    lw r2, -8(r13)
    ecall
    jal r0, after_ui

after_ui:
    jal r15, shell_paint
    jal r15, shell_compose
    jal r0, loop

idle:
    lw r1, -100(r13)
    addi r1, r1, 1
    sw r1, -100(r13)
    # Rare idle compose; viewer already presents each slice.
    addi r2, r0, 255
    and r1, r1, r2
    bne r1, r0, idle_y
    jal r15, shell_compose
idle_y:
    # M24.3: non-blocking reap of adopted/exited children (TRYWAIT).
    addi r1, r0, 52
    ecall
    addi r1, r0, 10
    ecall
    jal r0, loop

shutdown:
    jal r15, close_all
    addi r1, r0, 19
    lw r2, -8(r13)
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

# CLOSE every occupied run slot; WAIT once per closed child (never block if none).
close_all:
    addi r12, r15, 0
    addi r10, r0, 1
    addi r14, r0, 0
ca_scan:
    addi r1, r0, 3
    beq r10, r1, ca_reap
    addi r1, r0, 39
    addi r2, r10, 0
    ecall
    cmplt r3, r1, r0
    bne r3, r0, ca_next
    beq r2, r0, ca_next
    addi r11, r2, 0
    addi r1, r0, 40
    addi r2, r0, 1
    addi r3, r11, 0
    ecall
    addi r14, r14, 1
ca_next:
    addi r10, r10, 1
    jal r0, ca_scan
ca_reap:
    beq r14, r0, ca_done
    jal r15, yield_burst
    addi r1, r0, 7
    ecall
    addi r14, r14, -1
    jal r0, ca_reap
ca_done:
    jalr r0, r12, 0

yield_burst:
    addi r11, r15, 0
    addi r9, r0, 64
yb:
    addi r1, r0, 10
    ecall
    addi r9, r9, -1
    bne r9, r0, yb
    jalr r0, r11, 0

die:
    addi r1, r0, 0
    addi r2, r0, 0xe1
    ecall
    halt
