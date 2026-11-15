# text_terminal — prompt + EVENT_POLL; draws KEY_DOWN scancode glyph if any.
.isa 4
.global user_init

user_init:
    addi r2, r0, 2
    addi r3, r0, 2
    addi r4, r0, 48
    addi r5, r0, 20
    jal r6, base
    .word 0x00020000
base:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    addi r2, r8, 0
    jal r3, bg
    .word 0x00000000
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    addi r2, r8, 0
    addi r3, r0, 0
    addi r4, r0, 0
    jal r5, fg
    .word 0x00E0E0E0
fg:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, prompt
    .word 0x003E3E
prompt:
    jal r15, app_draw_text

    addi r9, r0, 0
    addi r11, r0, 8000
poll:
    jal r15, app_event_poll
    beq r1, r0, spin
    addi r4, r0, 1
    bne r1, r4, spin
    # KEY_DOWN: r3=scancode → printable-ish glyph '0'+ (code&15)
    jal r1, msk
    .word 0x0000000f
msk:
    lw r1, 0(r1)
    and r7, r3, r1
    addi r7, r7, 0x30
    addi r2, r8, 0
    addi r3, r0, 16
    addi r4, r0, 0
    jal r5, fg2
    .word 0x00E0E000
fg2:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r15, app_draw_char
    jal r0, done
spin:
    jal r15, app_yield
    addi r9, r9, 1
    cmplt r3, r9, r11
    bne r3, r0, poll
done:
    jal r15, app_compose
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf3
    jal r15, app_exit
