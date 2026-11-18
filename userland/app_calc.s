# Interactive calculator: digits, ops, Enter/=, Backspace, Esc.
# + : = key (0x0D) or keypad + (0x4E)
# - : main minus (0x0C) or keypad minus (0x4A)
# * : x/X (0x2D/0x2C)   / : slash (0x35)
# Window: top-left band so tiles at y>=88 stay clickable.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)
    addi r1, r0, 18
    addi r2, r0, 8
    addi r3, r0, 18
    addi r4, r0, 148
    addi r5, r0, 64
    jal r6, wbase
    .word 0x00060000
wbase:
    lw r6, 0(r6)
    ecall
    cmplt r3, r1, r0
    bne r3, r0, bad
    sw r1, -8(r13)
    sw r0, -12(r13)
    sw r0, -16(r13)
    sw r0, -20(r13)
    addi r1, r0, 1
    sw r1, -24(r13)
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
    # Save link on stack; paint/draw_num clobber r8–r11.
    sw r15, -40(r13)
    addi r1, r0, 0x0E
    beq r12, r1, do_bs
    addi r1, r0, 0x1C
    beq r12, r1, do_eq
    addi r1, r0, 0x4E
    beq r12, r1, op_add
    addi r1, r0, 0x0D
    beq r12, r1, op_add
    addi r1, r0, 0x4A
    beq r12, r1, op_sub
    addi r1, r0, 0x0C
    beq r12, r1, op_sub
    addi r1, r0, 0x2D
    beq r12, r1, op_mul
    addi r1, r0, 0x2C
    beq r12, r1, op_mul
    addi r1, r0, 0x35
    beq r12, r1, op_div
    addi r1, r0, 0x02
    cmplt r3, r12, r1
    bne r3, r0, ok_ret
    addi r1, r0, 0x0C
    cmplt r3, r12, r1
    beq r3, r0, ok_ret
    addi r2, r12, -2
    addi r1, r0, 9
    beq r2, r1, dig0
    addi r9, r2, 1
    jal r0, dig
dig0:
    addi r9, r0, 0
dig:
    lw r1, -24(r13)
    beq r1, r0, dig_rep
    lw r1, -16(r13)
    jal r15, mul10
    add r1, r1, r9
    jal r0, dig_store
dig_rep:
    addi r1, r9, 0
dig_store:
    jal r2, clim
    .word 1000000
clim:
    lw r2, 0(r2)
    cmplt r3, r1, r2
    bne r3, r0, dig_ok
    addi r1, r2, -1
dig_ok:
    sw r1, -16(r13)
    addi r1, r0, 1
    sw r1, -24(r13)
    jal r15, paint
    jal r0, ok_ret

do_bs:
    lw r1, -16(r13)
    jal r15, div10
    sw r1, -16(r13)
    addi r1, r0, 1
    sw r1, -24(r13)
    jal r15, paint
    jal r0, ok_ret

op_add:
    addi r9, r0, 1
    jal r0, apply_op
op_sub:
    addi r9, r0, 2
    jal r0, apply_op
op_mul:
    addi r9, r0, 3
    jal r0, apply_op
op_div:
    addi r9, r0, 4
apply_op:
    jal r15, reduce
    lw r1, -16(r13)
    sw r1, -12(r13)
    sw r9, -20(r13)
    sw r0, -24(r13)
    jal r15, paint
    jal r0, ok_ret

do_eq:
    jal r15, reduce
    sw r0, -20(r13)
    sw r0, -24(r13)
    jal r15, paint
ok_ret:
    lw r15, -40(r13)
    jalr r0, r15, 0

reduce:
    addi r10, r15, 0
    lw r1, -20(r13)
    beq r1, r0, red_done
    lw r2, -12(r13)
    lw r3, -16(r13)
    addi r4, r0, 1
    beq r1, r4, red_add
    addi r4, r0, 2
    beq r1, r4, red_sub
    addi r4, r0, 3
    beq r1, r4, red_mul
    beq r3, r0, red_err
    addi r1, r2, 0
    addi r2, r3, 0
    jal r15, idiv
    jal r0, red_store
red_add:
    add r1, r2, r3
    jal r0, red_store
red_sub:
    sub r1, r2, r3
    jal r0, red_store
red_mul:
    addi r1, r2, 0
    addi r2, r3, 0
    jal r15, imul
    jal r0, red_store
red_err:
    addi r1, r0, 0
red_store:
    sw r1, -16(r13)
    sw r0, -20(r13)
red_done:
    jalr r0, r10, 0

# Scratch: r2/r5/r6 only — must not clobber draw_num's r3/r4 (glyph x/y).
mul10:
    addi r2, r1, 0
    add r1, r1, r1
    add r5, r1, r1
    add r5, r5, r5
    add r1, r5, r2
    jalr r0, r15, 0

div10:
    addi r2, r0, 0
d10:
    addi r5, r0, 10
    cmplt r6, r1, r5
    bne r6, r0, d10d
    addi r1, r1, -10
    addi r2, r2, 1
    jal r0, d10
d10d:
    addi r1, r2, 0
    jalr r0, r15, 0

imul:
    addi r5, r0, 0
    beq r2, r0, imd
imo:
    add r5, r5, r1
    addi r2, r2, -1
    bne r2, r0, imo
imd:
    addi r1, r5, 0
    jalr r0, r15, 0

idiv:
    addi r5, r0, 0
idv:
    cmplt r6, r1, r2
    bne r6, r0, idd
    sub r1, r1, r2
    addi r5, r5, 1
    jal r0, idv
idd:
    addi r1, r5, 0
    jalr r0, r15, 0

# Draw one glyph: r7=char, r3=x, r4=y, uses -28 as 1-char C string.
glyph:
    sw r7, -28(r13)
    addi r1, r0, 28
    lw r2, -8(r13)
    jal r5, gfg
    .word 0x00A0FFA0
gfg:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -28
    ecall
    jalr r0, r15, 0

paint:
    addi r14, r15, 0
    addi r1, r0, 20
    lw r2, -8(r13)
    jal r3, bgc
    .word 0x00101820
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
    jal r7, t_calc
    .word 0x636C6143
    .word 0x00000000
t_calc:
    ecall
    lw r1, -20(r13)
    beq r1, r0, no_op
    addi r7, r0, 0x2B
    addi r2, r0, 1
    beq r1, r2, opch
    addi r7, r0, 0x2D
    addi r2, r0, 2
    beq r1, r2, opch
    addi r7, r0, 0x78
    addi r2, r0, 3
    beq r1, r2, opch
    addi r7, r0, 0x2F
opch:
    sw r7, -28(r13)
    addi r1, r0, 28
    lw r2, -8(r13)
    addi r3, r0, 128
    addi r4, r0, 6
    jal r5, fgo
    .word 0x00FFE080
fgo:
    lw r5, 0(r5)
    addi r6, r0, 0
    addi r7, r13, -28
    ecall
no_op:
    lw r1, -16(r13)
    addi r3, r0, 8
    addi r4, r0, 40
    jal r15, draw_num
    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    jalr r0, r14, 0

# r1=value, r3=x, r4=y — draw decimal with glyphs
draw_num:
    addi r8, r15, 0
    cmplt r5, r1, r0
    beq r5, r0, dn_pos
    addi r7, r0, 0x2D
    jal r15, glyph
    addi r3, r3, 8
    sub r1, r0, r1
dn_pos:
    beq r1, r0, dn_zero
    addi r10, r13, -128
    addi r11, r0, 0
dn_loop:
    beq r1, r0, dn_out
    addi r9, r1, 0
    jal r15, div10
    sw r1, -32(r13)
    jal r15, mul10
    sub r9, r9, r1
    addi r9, r9, 0x30
    sw r9, 0(r10)
    addi r10, r10, -4
    addi r11, r11, 1
    lw r1, -32(r13)
    jal r0, dn_loop
dn_out:
    addi r10, r10, 4
dn_emit:
    beq r11, r0, dn_done
    lw r7, 0(r10)
    jal r15, glyph
    addi r3, r3, 8
    addi r10, r10, 4
    addi r11, r11, -1
    jal r0, dn_emit
dn_zero:
    addi r7, r0, 0x30
    jal r15, glyph
dn_done:
    jalr r0, r8, 0

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
    addi r2, r0, 0xc1
    ecall
    halt
