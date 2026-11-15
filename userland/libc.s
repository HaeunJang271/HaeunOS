# Tiny guest libc helpers for UART output.
.isa 4
.global libc_putc
.global libc_puts
.global libc_nl

libc_putc:
    jal r1, p_u
    .word 0x10002000
p_u:
    lw r1, 0(r1)
    sw r2, 0(r1)
    addi r1, r0, 0
    jalr r0, r15, 0

libc_nl:
    addi r8, r15, 0
    addi r2, r0, 10
    jal r15, libc_putc
    jalr r0, r8, 0

# libc_puts: r2 = pointer to NUL-terminated word stream? uses byte chars in words.
# r10 = address of .word char list ending in 0
libc_puts:
    addi r8, r15, 0
    addi r10, r2, 0
    jal r1, p_u2
    .word 0x10002000
p_u2:
    lw r9, 0(r1)
ps_loop:
    lw r1, 0(r10)
    beq r1, r0, ps_done
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, ps_loop
ps_done:
    addi r1, r0, 0
    jalr r0, r8, 0
