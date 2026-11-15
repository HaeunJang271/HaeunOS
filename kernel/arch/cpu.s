# CPU architecture bring-up for the foundation kernel.
.isa 4
.global cpu_init

# Clears status.IE and leaves supervisor mode defaults. Returns K_OK (0).
cpu_init:
    addi r1, r0, 0
    csrrw r0, 0, r1
    addi r1, r0, 0
    jalr r0, r15, 0
