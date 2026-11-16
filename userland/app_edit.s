# Interactive editor: letters/digits, Space/Enter, Backspace, Esc.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)
    addi r1, r0, 18
    addi r2, r0, 88
    addi r3, r0, 40
    addi r4, r0, 176
    addi r5, r0, 96
    jal r6, wbase
    .word 0x00070000
wbase:
    lw r6, 0(r6)
    ecall
    cmplt r3, r1, r0
    bne r3, r0, bad
    sw r1, -8(r13)
    sw r0, -12(r13)
    jal r15, paint

loop:
    addi r1, r0, 27
    ecall
    beq r1, r0, idle
    addi r10, r0, 8
    beq r1, r10, bye
    addi r10, r0, 1
    bne r1, r10, loop
    addi r10, r0, 0x01
    beq r3, r10, bye
    addi r12, r3, 0
    jal r15, on_key
    jal r0, loop

idle:
    addi r1, r0, 37
    lw r2, -8(r13)
    ecall
    bne r1, r0, bye
    addi r1, r0, 10
    ecall
    jal r0, loop

on_key:
    # Save link on stack; paint clobbers r8–r11.
    sw r15, -40(r13)
    addi r1, r0, 0x0E
    beq r12, r1, do_bs
    jal r15, scan2asc
    beq r1, r0, ok_ret
    lw r2, -12(r13)
    addi r3, r0, 20
    cmplt r4, r2, r3
    beq r4, r0, ok_ret
    addi r3, r13, -128
    addi r4, r0, 0
ap_mul:
    beq r4, r2, ap_store
    addi r3, r3, -4
    addi r4, r4, 1
    jal r0, ap_mul
ap_store:
    sw r1, 0(r3)
    addi r2, r2, 1
    sw r2, -12(r13)
    jal r15, paint
    jal r0, ok_ret

do_bs:
    lw r2, -12(r13)
    beq r2, r0, ok_ret
    addi r2, r2, -1
    sw r2, -12(r13)
    jal r15, paint
ok_ret:
    lw r15, -40(r13)
    jalr r0, r15, 0

# r12=scancode → r1=ASCII or 0 (XT set-1 0x00..0x3F table).
scan2asc:
    addi r1, r0, 64
    cmplt r3, r12, r1
    beq r3, r0, s_no
    jal r1, kmap
    .word 0x00, 0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36
    .word 0x37, 0x38, 0x39, 0x30, 0x00, 0x00, 0x00, 0x00
    .word 0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69
    .word 0x6F, 0x70, 0x00, 0x00, 0x20, 0x00, 0x61, 0x73
    .word 0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x00
    .word 0x00, 0x00, 0x00, 0x00, 0x7A, 0x78, 0x63, 0x76
    .word 0x62, 0x6E, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00
    .word 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
kmap:
    addi r2, r0, 0
s_idx:
    beq r2, r12, s_hit
    addi r1, r1, 4
    addi r2, r2, 1
    jal r0, s_idx
s_hit:
    lw r1, 0(r1)
    jalr r0, r15, 0
s_no:
    addi r1, r0, 0
    jalr r0, r15, 0

paint:
    addi r14, r15, 0
    addi r1, r0, 20
    lw r2, -8(r13)
    jal r3, bgc
    .word 0x00202838
bgc:
    lw r3, 0(r3)
    ecall
    addi r1, r0, 28
    lw r2, -8(r13)
    addi r3, r0, 8
    addi r4, r0, 6
    jal r5, fgt
    .word 0x00E8F0FF
fgt:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, t_ed
    .word 0x74696445
    .word 0x0000726F
t_ed:
    ecall
    lw r11, -12(r13)
    addi r10, r13, -128
    addi r9, r0, 8
    addi r8, r0, 0
pn_loop:
    beq r8, r11, pn_done
    lw r7, 0(r10)
    sw r7, -28(r13)
    addi r1, r0, 28
    lw r2, -8(r13)
    addi r3, r9, 0
    addi r4, r0, 32
    jal r5, fgc
    .word 0x00FFE080
fgc:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -28
    ecall
    addi r9, r9, 8
    addi r10, r10, -4
    addi r8, r8, 1
    jal r0, pn_loop
pn_done:
    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    jalr r0, r14, 0

bye:
    addi r1, r0, 37
    lw r2, -8(r13)
    ecall
    bne r1, r0, bye_x
    addi r1, r0, 19
    lw r2, -8(r13)
    ecall
bye_x:
    lw r2, -4(r13)
    addi r1, r0, 0
    ecall
    halt

bad:
    addi r1, r0, 0
    addi r2, r0, 0xc2
    ecall
    halt
