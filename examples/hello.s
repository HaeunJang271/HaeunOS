# Minimal end-to-end HAEUN-32 guest program.
_start:
    addi r1, r0, 42
    halt

.data
.global sample_value
sample_value:
    .word 42
