# Guest GUI demo: WIN_CREATE / WIN_FILL / GFX_COMPOSE then halt.
# Requires haeun-vm --gui (host FB at 0x10000, 64x32) and ecall mux.
# Window backings: 0x18000 / 0x19000 / 0x1A000 (16x16 RGBA each).
.isa 4
.global _start

_start:
    # WIN_CREATE red window at (0,0) 16x16 backing 0x18000
    addi r1, r0, 18
    addi r2, r0, 0
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x18000
    ecall
    addi r8, r1, 0

    # WIN_FILL id=r8 color=0x000000FF (R)
    addi r1, r0, 20
    addi r2, r8, 0
    addi r3, r0, 0xFF
    ecall

    # WIN_CREATE green window at (16,0)
    addi r1, r0, 18
    addi r2, r0, 16
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x19000
    ecall
    addi r9, r1, 0

    # WIN_FILL green 0x0000FF00
    addi r1, r0, 20
    addi r2, r9, 0
    jal r10, load_g
    .word 0x0000FF00
load_g:
    lw r3, 0(r10)
    ecall

    # WIN_CREATE blue window at (32,0)
    addi r1, r0, 18
    addi r2, r0, 32
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x1A000
    ecall
    addi r11, r1, 0

    # WIN_FILL blue 0x00FF0000
    addi r1, r0, 20
    addi r2, r11, 0
    jal r10, load_b
    .word 0x00FF0000
load_b:
    lw r3, 0(r10)
    ecall

    # GFX_COMPOSE desktop black
    addi r1, r0, 23
    addi r2, r0, 0
    ecall

    addi r1, r0, 0
    halt
