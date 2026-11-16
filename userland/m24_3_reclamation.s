# M24.3 E: fill AS with two children, reap, respawn; TRYWAIT idle; SPAWN-fallback note.
# Markers: F=full, V=vacated/respawn ok, D=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 5
    ecall
    beq r1, r0, c1
    addi r1, r0, 5
    ecall
    beq r1, r0, c2
    # Third FORK must fail (init+2 children = AS cap).
    addi r1, r0, 5
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x46
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
    # Slot free: FORK again.
    addi r1, r0, 5
    ecall
    beq r1, r0, c3
    cmplt r3, r1, r0
    bne r3, r0, fail
    addi r1, r0, 7
    ecall
    # Idle TRYWAIT empty.
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, fail
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x56
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x44
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

fail:
    addi r1, r0, 0
    addi r2, r0, 0xEF
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

c3:
    addi r1, r0, 0
    addi r2, r0, 3
    ecall
    halt
