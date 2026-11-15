# Terminal halt used by kpanic; never returns.
.isa 4
.global panic_halt

panic_halt:
    addi r1, r2, 0
    halt
