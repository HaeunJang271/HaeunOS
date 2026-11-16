# M22.1.3+ 320x180 desktop paint (header / welcome / tiles / taskbar).
# Stack: win -8; busy -92; menu -96; paint uses -108..-140.
.isa 4
.global shell_paint
.global shell_compose
.global shell_fill_rect
.global shell_text
.global shell_tile

shell_compose:
    addi r1, r0, 23
    jal r2, cc_clr
    .word 0x00141C28
cc_clr:
    lw r2, 0(r2)
    ecall
    jalr r0, r15, 0

shell_paint:
    sw r15, -108(r13)
    addi r1, r0, 20
    lw r2, -8(r13)
    jal r3, c_bg
    .word 0x00141C28
c_bg:
    lw r3, 0(r3)
    ecall
    addi r3, r0, 0
    addi r4, r0, 0
    addi r5, r0, 320
    addi r6, r0, 20
    jal r7, c_hd
    .word 0x00182028
c_hd:
    lw r7, 0(r7)
    jal r15, shell_fill_rect
    addi r3, r0, 8
    addi r4, r0, 6
    jal r5, c_fg
    .word 0x00F0F4FF
c_fg:
    lw r5, 0(r5)
    jal r7, t_os
    .word 0x55454148
    .word 0x534F2D4E
    .word 0x00000000
t_os:
    jal r15, shell_text
    addi r3, r0, 216
    addi r4, r0, 6
    jal r5, c_sr
    .word 0x00A0A8B0
c_sr:
    lw r5, 0(r5)
    jal r7, t_rdy
    .word 0x54535953
    .word 0x52204D45
    .word 0x59444145
    .word 0x00000000
t_rdy:
    jal r15, shell_text
    addi r3, r0, 12
    addi r4, r0, 28
    addi r5, r0, 296
    addi r6, r0, 48
    jal r7, c_wp
    .word 0x00283040
c_wp:
    lw r7, 0(r7)
    jal r15, shell_fill_rect
    addi r3, r0, 24
    addi r4, r0, 36
    jal r5, c_wf
    .word 0x00F0F4FF
c_wf:
    lw r5, 0(r5)
    jal r7, t_wel
    .word 0x636C6557
    .word 0x2E656D6F
    .word 0x00000000
t_wel:
    jal r15, shell_text
    addi r3, r0, 24
    addi r4, r0, 52
    jal r5, c_ws
    .word 0x00A0A8B0
c_ws:
    lw r5, 0(r5)
    jal r7, t_sub
    .word 0x72756F59
    .word 0x726F7720
    .word 0x6170736B
    .word 0x69206563
    .word 0x65722073
    .word 0x2E796461
    .word 0x00000000
t_sub:
    jal r15, shell_text
    addi r3, r0, 16
    addi r4, r0, 88
    jal r5, c_a0
    .word 0x0040C080
c_a0:
    lw r5, 0(r5)
    jal r6, c_c0
    .word 0x00000043
c_c0:
    lw r6, 0(r6)
    jal r7, t_n0
    .word 0x636C6143
    .word 0x74616C75
    .word 0x0000726F
t_n0:
    jal r15, shell_tile
    addi r3, r0, 112
    addi r4, r0, 88
    jal r5, c_a1
    .word 0x0030A0A0
c_a1:
    lw r5, 0(r5)
    jal r6, c_c1
    .word 0x00000045
c_c1:
    lw r6, 0(r6)
    jal r7, t_n1
    .word 0x74696445
    .word 0x0000726F
t_n1:
    jal r15, shell_tile
    addi r3, r0, 208
    addi r4, r0, 88
    jal r5, c_a2
    .word 0x00C08040
c_a2:
    lw r5, 0(r5)
    jal r6, c_c2
    .word 0x00000046
c_c2:
    lw r6, 0(r6)
    jal r7, t_n2
    .word 0x656C6946
    .word 0x00000073
t_n2:
    jal r15, shell_tile
    addi r3, r0, 0
    addi r4, r0, 164
    addi r5, r0, 320
    addi r6, r0, 16
    jal r7, c_bar
    .word 0x00040C10
c_bar:
    lw r7, 0(r7)
    jal r15, shell_fill_rect
    addi r3, r0, 8
    addi r4, r0, 168
    jal r5, c_bf
    .word 0x00E8F0FF
c_bf:
    lw r5, 0(r5)
    jal r7, t_bn
    .word 0x55454148
    .word 0x0000004E
t_bn:
    jal r15, shell_text
    addi r3, r0, 248
    addi r4, r0, 168
    jal r5, c_bd
    .word 0x00E8F0FF
c_bd:
    lw r5, 0(r5)
    jal r7, t_bd
    .word 0x6B736544
    .word 0x00706F74
t_bd:
    jal r15, shell_text
    lw r1, -92(r13)
    beq r1, r0, done
    addi r3, r0, 300
    addi r4, r0, 6
    addi r5, r0, 10
    addi r6, r0, 8
    jal r7, c_bs
    .word 0x00FFE080
c_bs:
    lw r7, 0(r7)
    jal r15, shell_fill_rect
done:
    lw r15, -108(r13)
    jalr r0, r15, 0

# r3,r4 origin; r5 accent; r6 char code; r7 label VA. 96x56 tile.
shell_tile:
    sw r15, -112(r13)
    sw r3, -116(r13)
    sw r4, -120(r13)
    sw r5, -124(r13)
    sw r6, -128(r13)
    sw r7, -132(r13)
    addi r5, r0, 96
    addi r6, r0, 56
    jal r7, c_tb
    .word 0x00202838
c_tb:
    lw r7, 0(r7)
    jal r15, shell_fill_rect
    lw r3, -116(r13)
    addi r3, r3, 8
    lw r4, -120(r13)
    addi r4, r4, 8
    addi r5, r0, 24
    addi r6, r0, 16
    lw r7, -124(r13)
    jal r15, shell_fill_rect
    addi r1, r0, 28
    lw r2, -8(r13)
    lw r3, -116(r13)
    addi r3, r3, 12
    lw r4, -120(r13)
    addi r4, r4, 10
    jal r5, c_tc
    .word 0x00F0F4FF
c_tc:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -128
    ecall
    addi r1, r0, 28
    lw r2, -8(r13)
    lw r3, -116(r13)
    addi r3, r3, 8
    lw r4, -120(r13)
    addi r4, r4, 36
    jal r5, c_tl
    .word 0x00E8F0FF
c_tl:
    lw r5, 0(r5)
    addi r6, r0, 0
    lw r7, -132(r13)
    ecall
    lw r15, -112(r13)
    jalr r0, r15, 0

shell_text:
    sw r15, -112(r13)
    addi r1, r0, 28
    lw r2, -8(r13)
    addi r6, r0, 0
    ecall
    lw r15, -112(r13)
    jalr r0, r15, 0

# r3=x r4=y r5=w r6=h r7=rgba — host WIN_FILL_RECT (51).
shell_fill_rect:
    sw r15, -136(r13)
    addi r1, r0, 51
    lw r2, -8(r13)
    ecall
    lw r15, -136(r13)
    jalr r0, r15, 0
