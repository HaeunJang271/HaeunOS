# Text editor: create/open note.txt on HFS, write 'H''I', read back, paint window.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    addi r1, r0, 18
    addi r2, r0, 12
    addi r3, r0, 6
    addi r4, r0, 24
    addi r5, r0, 12
    jal r6, base
    .word 0x00022000
base:
    lw r6, 0(r6)
    ecall
    addi r8, r1, 0

    # OPEN path "note.txt" create (r2 -> NUL string)
    jal r2, open_c
    .word 0x65746f6e
    .word 0x7478742e
    .word 0x00000000
open_c:
    addi r1, r0, 3
    addi r3, r0, 1
    ecall
    cmplt r3, r1, r0
    bne r3, r0, paint_fail
    addi r9, r1, 0

    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 72
    ecall
    addi r1, r0, 1
    addi r2, r9, 0
    addi r3, r0, 73
    ecall

    addi r1, r0, 4
    addi r2, r9, 0
    ecall

    jal r2, open_r
    .word 0x65746f6e
    .word 0x7478742e
    .word 0x00000000
open_r:
    addi r1, r0, 3
    addi r3, r0, 0
    ecall
    addi r9, r1, 0

    addi r1, r0, 2
    addi r2, r9, 0
    ecall
    addi r4, r0, 72
    bne r1, r4, paint_fail

    addi r1, r0, 4
    addi r2, r9, 0
    ecall

    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, ok
    .word 0x00004080
ok:
    lw r3, 0(r3)
    ecall
    addi r1, r0, 23
    addi r2, r0, 0
    ecall
    addi r1, r0, 19
    addi r2, r8, 0
    ecall
    lw r2, -4(r13)
    addi r1, r0, 0
    ecall
    halt
paint_fail:
    addi r1, r0, 20
    addi r2, r8, 0
    jal r3, er
    .word 0x000000A0
er:
    lw r3, 0(r3)
    ecall
    addi r1, r0, 0
    addi r2, r0, 0xc2
    ecall
    halt
