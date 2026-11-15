# GUI demo as PCB0 user process (linked at VA 0x00108000).
# Lifecycle: GETPID → WIN_* → GFX_COMPOSE → EXIT (kernel trap).
# GUI ECALL 18–23 completed by host assist; EXIT uses process trap.
.isa 4
.global user_init

user_init:
    # SYS_GETPID → r8
    addi r1, r0, 9
    ecall
    addi r8, r1, 0

    # WIN_CREATE red (0,0) 16x16 backing 0x18000
    addi r1, r0, 18
    addi r2, r0, 0
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x18000
    ecall
    addi r9, r1, 0

    addi r1, r0, 20
    addi r2, r9, 0
    addi r3, r0, 0xFF
    ecall

    # WIN_CREATE green (16,0)
    addi r1, r0, 18
    addi r2, r0, 16
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x19000
    ecall
    addi r10, r1, 0

    addi r1, r0, 20
    addi r2, r10, 0
    jal r11, load_g
    .word 0x0000FF00
load_g:
    lw r3, 0(r11)
    ecall

    # WIN_CREATE blue (32,0)
    addi r1, r0, 18
    addi r2, r0, 32
    addi r3, r0, 0
    addi r4, r0, 16
    addi r5, r0, 16
    addi r6, r0, 0x1A000
    ecall
    addi r12, r1, 0

    addi r1, r0, 20
    addi r2, r12, 0
    jal r11, load_b
    .word 0x00FF0000
load_b:
    lw r3, 0(r11)
    ecall

    # GFX_COMPOSE black desk
    addi r1, r0, 23
    addi r2, r0, 0
    ecall

    # SYS_EXIT(pid) — leave pid in exit_code for host PCB check
    addi r1, r0, 0
    addi r2, r8, 0
    ecall
    halt
