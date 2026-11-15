# M10 input demo (flat VA=PA): GUI windows + poll KBD/MOUSE MMIO.
# Host --gui --user --input feeds make/break then a click on the green window.
.isa 4
.global _start

_start:
    # WIN_CREATE red (0,0) 16x16 @0x18000
    addi r1, r0, 18
    addi r2, r0, 0
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x18000
    ecall
    addi r8, r1, 0
    addi r1, r0, 20
    addi r2, r8, 0
    addi r3, r0, 0xFF
    ecall

    # WIN_CREATE green (16,0)
    addi r1, r0, 18
    addi r2, r0, 16
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x19000
    ecall
    addi r9, r1, 0
    addi r1, r0, 20
    addi r2, r9, 0
    jal r10, load_g
    .word 0x0000FF00
load_g:
    lw r3, 0(r10)
    ecall

    addi r1, r0, 23
    addi r2, r0, 0
    ecall

    # Poll keyboard STATUS @0x10005004 until make 0x1C then break 0x9C
    jal r12, kbd_base
    .word 0x10005000
kbd_base:
    lw r12, 0(r12)
wait_make:
    lw r1, 4(r12)
    beq r1, r0, wait_make
    lw r1, 0(r12)
    addi r2, r0, 0x1C
    bne r1, r2, bad
wait_break:
    lw r1, 4(r12)
    beq r1, r0, wait_break
    lw r1, 0(r12)
    addi r2, r0, 0x9C
    bne r1, r2, bad

    # Poll mouse STATUS @0x10006000
    jal r12, mouse_base
    .word 0x10006000
mouse_base:
    lw r12, 0(r12)
wait_mouse:
    lw r1, 0(r12)
    beq r1, r0, wait_mouse
    lw r1, 4(r12)
    addi r2, r0, 20
    bne r1, r2, bad
    lw r1, 8(r12)
    addi r2, r0, 4
    bne r1, r2, bad
    lw r1, 12(r12)
    addi r2, r0, 1
    bne r1, r2, bad
    sw r0, 20(r12)

    addi r1, r0, 1
    halt
bad:
    addi r1, r0, 0
    halt
