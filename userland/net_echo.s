# UDP echo via real host backend: SOCKET/BIND/SENDTO 127.0.0.1:19007 / RECVFROM / CLOSE / EXIT.
.isa 4
.global user_init

user_init:
    addi r1, r0, 9
    ecall
    sw r1, -4(r13)

    # SOCKET DGRAM
    addi r1, r0, 11
    addi r2, r0, 1
    ecall
    addi r8, r1, 0

    # BIND port 7
    addi r1, r0, 12
    addi r2, r8, 0
    addi r3, r0, 7
    ecall

    # payload "HI" on stack
    jal r1, pay
    .word 0x00004948
pay:
    lw r2, 0(r1)
    sw r2, -8(r13)

    # SENDTO dst 127.0.0.1:19007 buf=sp-8 len=2
    addi r1, r0, 13
    addi r2, r8, 0
    jal r3, dip
    .word 0x7f000001
dip:
    lw r3, 0(r3)
    addi r4, r0, 19007
    addi r5, r13, -8
    addi r6, r0, 2
    ecall

    # RECVFROM into sp-16 (many retries for host round-trip)
    addi r9, r0, 64
recv_try:
    addi r1, r0, 14
    addi r2, r8, 0
    addi r3, r13, -16
    addi r4, r0, 8
    addi r5, r0, 0
    ecall
    cmplt r10, r1, r0
    beq r10, r0, recv_ok
    addi r9, r9, -1
    bne r9, r0, recv_try
    addi r1, r0, 0
    addi r2, r0, 0xee
    ecall
    halt
recv_ok:
    addi r2, r0, 2
    bne r1, r2, recv_bad
    # verify little-endian word starts with 'H''I' (0x00004948)
    lw r2, -16(r13)
    jal r3, expect
    .word 0x00004948
expect:
    lw r3, 0(r3)
    bne r2, r3, recv_bad
    # SO_CLOSE
    addi r1, r0, 17
    addi r2, r8, 0
    ecall
    lw r2, -4(r13)
    addi r1, r0, 0
    ecall
    halt
recv_bad:
    addi r1, r0, 0
    addi r2, r0, 0xef
    ecall
    halt
