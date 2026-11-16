# M25.4: valid stack edges OK; child touches guard → fault; parent continues.
# Markers: N=normal edges, G=parent after child guard fault, Z=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    # Normal: store at top-4 and at ustack_base (r14).
    addi r13, r13, -4
    addi r1, r0, 0x4E
    sw r1, 0(r13)
    sw r1, 0(r14)
    lw r2, 0(r13)
    bne r1, r2, fail
    lw r2, 0(r14)
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x4E
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_guard
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x47
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

child_guard:
    # First word of guard page = ustack_base - 4 (slot0: 0x112FFC).
    addi r1, r14, -4
    sw r0, 0(r1)
    addi r1, r0, 0
    addi r2, r0, 0xE0
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt
