# M23.2: one guest creates two windows; raise/focus/destroy/recreate.
.isa 4
.global user_init

user_init:
    # Window A: (20,40) 40x24 @ 0x20000
    addi r2, r0, 20
    addi r3, r0, 40
    addi r4, r0, 40
    addi r5, r0, 24
    jal r6, base_a
    .word 0x00020000
base_a:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad

    # Window B: (36,48) 40x24 @ 0x28000 (overlaps A)
    addi r2, r0, 36
    addi r3, r0, 48
    addi r4, r0, 40
    addi r5, r0, 24
    jal r6, base_b
    .word 0x00028000
base_b:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r9, r1, 0
    cmplt r3, r9, r0
    bne r3, r0, bad
    beq r8, r9, bad

    # Distinct fills (RGBA word: R/G/B/A little-endian lanes)
    addi r2, r8, 0
    jal r3, col_red
    .word 0x000000ff
col_red:
    lw r3, 0(r3)
    jal r15, app_win_fill
    addi r2, r9, 0
    jal r3, col_grn
    .word 0x0000ff00
col_grn:
    lw r3, 0(r3)
    jal r15, app_win_fill

    # Raise A → focus A; raise B → focus B
    addi r2, r8, 0
    jal r15, app_win_raise
    addi r2, r8, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r3, r0, 1
    and r3, r5, r3
    beq r3, r0, bad

    addi r2, r9, 0
    jal r15, app_win_raise
    addi r2, r9, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r3, r0, 1
    and r3, r5, r3
    beq r3, r0, bad

    jal r2, desk
    .word 0x203040ff
desk:
    lw r2, 0(r2)
    jal r15, app_compose

    # Destroy A; B must remain queryable and focused after recovery
    addi r2, r8, 0
    jal r15, app_win_destroy
    addi r2, r9, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r3, r0, 1
    and r3, r5, r3
    beq r3, r0, bad

    # Stale id A must fail query
    addi r2, r8, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    beq r3, r0, bad

    # Recreate A (may reuse slot); must be distinct from live B
    addi r2, r0, 20
    addi r3, r0, 40
    addi r4, r0, 40
    addi r5, r0, 24
    jal r6, base_a2
    .word 0x00020000
base_a2:
    lw r6, 0(r6)
    jal r15, app_win_create
    addi r8, r1, 0
    cmplt r3, r8, r0
    bne r3, r0, bad
    beq r8, r9, bad
    addi r2, r8, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad
    addi r2, r9, 0
    jal r15, app_win_query
    cmplt r3, r1, r0
    bne r3, r0, bad

    # Final raise B + compose so host PPM sees green on top in overlap.
    addi r2, r8, 0
    jal r3, col_red2
    .word 0x000000ff
col_red2:
    lw r3, 0(r3)
    jal r15, app_win_fill
    addi r2, r9, 0
    jal r3, col_grn2
    .word 0x0000ff00
col_grn2:
    lw r3, 0(r3)
    jal r15, app_win_fill
    addi r2, r9, 0
    jal r15, app_win_raise
    jal r2, desk2
    .word 0x203040ff
desk2:
    lw r2, 0(r2)
    jal r15, app_compose

    addi r2, r0, 0
    jal r15, app_exit
bad:
    addi r2, r0, 0xf7
    jal r15, app_exit
