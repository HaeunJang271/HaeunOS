# Early exception vector install for the foundation kernel.
.isa 4
.global trap_init
.global early_trap

# Installs early_trap into trapvec via PC-relative address. Returns K_OK.
trap_init:
    jal r1, here
here:
    addi r1, r1, 16
    csrrw r0, 1, r1
    addi r1, r0, 0
    jalr r0, r15, 0
early_trap:
    addi r1, r0, 2
    halt
