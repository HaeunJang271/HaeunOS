# M12 desktop demo: three overlapping windows then halt.
# Host --desktop --viewer supplies chrome (title/drag/resize/min/max/close).
.isa 4
.global _start

_start:
    # Window A red (2,8) 18x12 @0x18000
    addi r1, r0, 18
    addi r2, r0, 2
    addi r3, r0, 8
    addi r4, r0, 18
    addi r5, r0, 12
    addi r6, r0, 0x18000
    ecall
    addi r8, r1, 0
    addi r1, r0, 20
    addi r2, r8, 0
    addi r3, r0, 0xFF
    ecall

    # Window B green (14,10) 18x12 @0x19000
    addi r1, r0, 18
    addi r2, r0, 14
    addi r3, r0, 10
    addi r4, r0, 18
    addi r5, r0, 12
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

    # Window C blue (26,12) 18x12 @0x1A000
    addi r1, r0, 18
    addi r2, r0, 26
    addi r3, r0, 12
    addi r4, r0, 18
    addi r5, r0, 12
    addi r6, r0, 0x1A000
    ecall
    addi r11, r1, 0
    addi r1, r0, 20
    addi r2, r11, 0
    jal r10, load_b
    .word 0x00FF0000
load_b:
    lw r3, 0(r10)
    ecall

    # Raise middle window for overlap demo
    addi r1, r0, 22
    addi r2, r9, 0
    ecall

    addi r1, r0, 23
    addi r2, r0, 0
    ecall

    addi r1, r0, 1
    halt
