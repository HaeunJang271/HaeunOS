# Init process: print banner and enter the shell.
.isa 4
.global _start
.global shell_main
.global libc_puts
.global libc_nl

_start:
    jal r15, print_init
    jal r15, shell_main
    addi r1, r0, 0
    halt

print_init:
    addi r8, r15, 0
    jal r1, pu
    .word 0x10002000
pu:
    lw r9, 0(r1)
    jal r10, loop
    .word 105
    .word 110
    .word 105
    .word 116
    .word 10
    .word 0
loop:
    lw r1, 0(r10)
    beq r1, r0, done
    sw r1, 0(r9)
    addi r10, r10, 4
    jal r0, loop
done:
    jalr r0, r8, 0
