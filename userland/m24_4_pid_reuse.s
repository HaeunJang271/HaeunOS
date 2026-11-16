# M24.4 C: FORK/EXIT/WAIT then FORK again; foreign zombie not reaped by init TRYWAIT.
# Markers: A first reap, B second pid path, X foreign protected, Z done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    # Cycle 1
    addi r1, r0, 5
    ecall
    beq r1, r0, c1
    addi r9, r1, 0
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x41
    ecall
    # Cycle 2: new child after FREE
    addi r1, r0, 5
    ecall
    beq r1, r0, c2
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x42
    ecall
    # Foreign: sub-parent owns zombie; init TRYWAIT must be -1
    addi r1, r0, 5
    ecall
    beq r1, r0, sub_parent
    addi r1, r0, 10
    ecall
    addi r1, r0, 10
    ecall
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x58
    ecall
    # Let sub_parent WAIT then exit
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt

c1:
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

c2:
    addi r1, r0, 0
    addi r2, r0, 2
    ecall
    halt

sub_parent:
    addi r1, r0, 5
    ecall
    beq r1, r0, sub_child
    # Yield so child EXITs to ZOMBIE while we still live
    addi r1, r0, 10
    ecall
    addi r1, r0, 10
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 0
    addi r2, r0, 4
    ecall
    halt

sub_child:
    addi r1, r0, 0
    addi r2, r0, 5
    ecall
    halt
