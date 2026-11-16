# M24.3: single + multi orphan reaped by init via WAIT; idle TRYWAIT.
# Markers: S=start, P=parent gone path, A/B/C=reaped, I=idle ok, Z=done.
.isa 4
.global user_init

.align 4096
user_init:
    addi r1, r0, 3
    addi r2, r0, 1
    ecall
    addi r8, r1, 0
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x53
    ecall
    # --- A: single orphan ---
    addi r1, r0, 5
    ecall
    beq r1, r0, parent_a
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x41
    ecall
    # --- B: two orphans sequential ---
    addi r1, r0, 5
    ecall
    beq r1, r0, parent_b
    addi r1, r0, 7
    ecall
    addi r1, r0, 7
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x42
    ecall
    # --- D: idle TRYWAIT with no zombies ---
    addi r4, r0, 8
idle_loop:
    addi r1, r0, 52
    ecall
    addi r2, r0, -1
    bne r1, r2, idle_bad
    addi r1, r0, 10
    ecall
    addi r4, r4, -1
    bne r4, r0, idle_loop
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x49
    ecall
    addi r1, r0, 1
    addi r2, r8, 0
    addi r3, r0, 0x5A
    ecall
    addi r1, r0, 0
    addi r2, r0, 0
    ecall
    halt

idle_bad:
    addi r1, r0, 0
    addi r2, r0, 0xEE
    ecall
    halt

parent_a:
    addi r1, r0, 5
    ecall
    beq r1, r0, child_a
    addi r1, r0, 0
    addi r2, r0, 1
    ecall
    halt

child_a:
    addi r1, r0, 0
    addi r2, r0, 11
    ecall
    halt

parent_b:
    addi r1, r0, 5
    ecall
    beq r1, r0, child_b1
    # Wait for first child to free AS before second FORK.
    addi r1, r0, 7
    ecall
    addi r1, r0, 5
    ecall
    beq r1, r0, child_b2
    addi r1, r0, 0
    addi r2, r0, 2
    ecall
    halt

child_b1:
    addi r1, r0, 0
    addi r2, r0, 21
    ecall
    halt

child_b2:
    addi r1, r0, 0
    addi r2, r0, 22
    ecall
    halt
