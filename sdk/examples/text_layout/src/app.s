# text_layout — positioned text, long wrap, empty string, edge clip.
.isa 4
.global user_init

user_init:
    addi r2, r0, 0
    addi r3, r0, 0
    addi r4, r0, 40
    addi r5, r0, 24
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
    .word 0x00202020
bg:
    lw r3, 0(r3)
    jal r15, app_win_fill

    # empty string at (0,0)
    addi r2, r8, 0
    addi r3, r0, 0
    addi r4, r0, 0
    jal r5, fg
    .word 0x00FFFFFF
fg:
    lw r5, 0(r5)
    addi r6, r0, 0
    jal r7, empty
    .word 0x00000000
empty:
    jal r15, app_draw_text

    # long string wraps inside 40px (5 glyphs/line)
    addi r2, r8, 0
    addi r3, r0, 0
    addi r4, r0, 0
    jal r5, fg2
    .word 0x00FFFFFF
fg2:
    lw r5, 0(r5)
    jal r6, bg2
    .word 0x00202020
bg2:
    lw r6, 0(r6)
    jal r7, long
    .word 0x44434241
    .word 0x48474645
    .word 0x00004A49
long:
    jal r15, app_draw_text

    # near bottom-right: clips
    addi r2, r8, 0
    addi r3, r0, 36
    addi r4, r0, 20
    jal r5, fg3
    .word 0x00FF8080
fg3:
    lw r5, 0(r5)
    jal r6, bg3
    .word 0x00202020
bg3:
    lw r6, 0(r6)
    jal r7, edge
    .word 0x00005858
edge:
    jal r15, app_draw_text

    jal r15, app_compose
    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf2
    jal r15, app_exit
