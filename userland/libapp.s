# Reusable guest wrappers for process / window / app / HFS ECALLs.
# Convention: args in r2..; return in r1; r15 = link (jalr return).
.isa 4
.global app_exit
.global app_getpid
.global app_yield
.global app_wait
.global app_win_create
.global app_win_destroy
.global app_win_fill
.global app_win_pixel
.global app_win_raise
.global app_compose
.global app_count
.global app_spawn
.global app_putc
.global app_open
.global app_read
.global app_write
.global app_close
.global app_event_poll
.global app_event_wait
.global app_event_type
.global app_evt_iswin
.global app_win_query
.global app_draw_text
.global app_draw_char
.global app_ipc_create
.global app_ipc_send
.global app_ipc_recv
.global app_ipc_close
.global app_ipc_avail
.global app_ipc_peek
.global app_ipc_sender
.global app_svc_reg
.global app_svc_lookup
.global app_info
.global app_run
.global app_ctrl
.global app_name

app_exit:
    addi r1, r0, 0
    ecall
    halt

app_getpid:
    addi r1, r0, 9
    ecall
    jalr r0, r15, 0

app_yield:
    addi r1, r0, 10
    ecall
    jalr r0, r15, 0

app_wait:
    addi r1, r0, 7
    ecall
    jalr r0, r15, 0

# r2=x r3=y r4=w r5=h r6=base → r1=id
app_win_create:
    addi r1, r0, 18
    ecall
    jalr r0, r15, 0

app_win_destroy:
    addi r1, r0, 19
    ecall
    jalr r0, r15, 0

# r2=id r3=color
app_win_fill:
    addi r1, r0, 20
    ecall
    jalr r0, r15, 0

# r2=id r3=x r4=y r5=color
app_win_pixel:
    addi r1, r0, 21
    ecall
    jalr r0, r15, 0

# r2=id
app_win_raise:
    addi r1, r0, 22
    ecall
    jalr r0, r15, 0

app_compose:
    addi r1, r0, 23
    ecall
    jalr r0, r15, 0

app_count:
    addi r1, r0, 24
    ecall
    jalr r0, r15, 0

# r2=app index → r1=pid
app_spawn:
    addi r1, r0, 26
    ecall
    jalr r0, r15, 0

# r2=index r3=buf VA r4=cap → r1=0/-1 (APP_NAME)
app_name:
    addi r1, r0, 25
    ecall
    jalr r0, r15, 0

# r2=index r3=field r4=buf VA r5=cap → r1=0/-1; FLAGS: r2=flags
app_info:
    addi r1, r0, 38
    ecall
    jalr r0, r15, 0

# r2=slot → r1=0; r2=pid r3=app_index r4=state|(exit<<16) r5=win
app_run:
    addi r1, r0, 39
    ecall
    jalr r0, r15, 0

# r2=op r3=pid → r1=0/-1
app_ctrl:
    addi r1, r0, 40
    ecall
    jalr r0, r15, 0

# r2=byte → console WRITE (ECALL); UART MMIO is not U-mapped in process-v1.
app_putc:
    addi r3, r2, 0
    addi r2, r0, 1
    addi r1, r0, 1
    ecall
    jalr r0, r15, 0

# OPEN: r2=1 console, or r2=path VA + r3!=0 create → r1=fd (r2/r3 pass-through)
app_open:
    addi r1, r0, 3
    ecall
    jalr r0, r15, 0

# r2=fd → r1=byte or -1
app_read:
    addi r1, r0, 2
    ecall
    jalr r0, r15, 0

# r2=fd r3=byte
app_write:
    addi r1, r0, 1
    ecall
    jalr r0, r15, 0

# r2=fd
app_close:
    addi r1, r0, 4
    ecall
    jalr r0, r15, 0

# → r1=type (0 empty); on hit also r2=win r3=a r4=b
app_event_poll:
    addi r1, r0, 27
    ecall
    jalr r0, r15, 0

# Cooperative wait: YIELD then EVENT_POLL until non-empty (or caller bounds).
# Clobbers r1..r4 like poll; returns same ABI as app_event_poll.
app_event_wait:
    addi r10, r15, 0
ewait_loop:
    jal r15, app_yield
    jal r15, app_event_poll
    beq r1, r0, ewait_loop
    jalr r0, r10, 0

# r1=type from last poll → r1 unchanged (identity helper for apps)
app_event_type:
    jalr r0, r15, 0

# r1=type → r1=1 if window event (preserves r2..r4 via r8/r9 temps)
app_evt_iswin:
    addi r8, r1, 0
    addi r9, r0, 6
    cmplt r1, r8, r9
    bne r1, r0, eiw_no
    addi r9, r0, 10
    beq r8, r9, eiw_no
    addi r9, r0, 18
    cmplt r1, r8, r9
    beq r1, r0, eiw_no
    addi r1, r0, 1
    jalr r0, r15, 0
eiw_no:
    addi r1, r0, 0
    jalr r0, r15, 0

# r2=id → r1=0; r2=x r3=y r4=(w|(h<<16)) r5=flags
app_win_query:
    addi r1, r0, 37
    ecall
    jalr r0, r15, 0

# r2=id r3=x r4=y r5=fg r6=bg r7=str VA (HOBJ name ≤15 chars)
app_draw_text:
    addi r1, r0, 28
    ecall
    jalr r0, r15, 0

# r2=id r3=x r4=y r5=fg r6=bg r7=char code; one glyph via WIN_DRAW_TEXT
app_draw_char:
    addi r10, r15, 0
    jal r1, chmask
    .word 0x000000ff
chmask:
    lw r1, 0(r1)
    and r7, r7, r1
    sw r7, -8(r13)
    addi r7, r13, -8
    addi r1, r0, 28
    ecall
    jalr r0, r10, 0

app_ipc_create:
    addi r1, r0, 29
    ecall
    jalr r0, r15, 0

# r2=epid r3=buf VA r4=len
app_ipc_send:
    addi r1, r0, 30
    ecall
    jalr r0, r15, 0

# r2=epid r3=buf VA r4=cap → r1=len; r2=sender
app_ipc_recv:
    addi r1, r0, 31
    ecall
    jalr r0, r15, 0

# r2=epid
app_ipc_close:
    addi r1, r0, 32
    ecall
    jalr r0, r15, 0

# r2=epid → r1=queued count (or -1)
app_ipc_avail:
    addi r1, r0, 35
    ecall
    jalr r0, r15, 0

# r2=epid → r1=len (0 empty); r2=sender r3=type r4=seq
app_ipc_peek:
    addi r1, r0, 36
    ecall
    jalr r0, r15, 0

# After app_ipc_recv/peek: r2 already holds sender; identity helper.
app_ipc_sender:
    jalr r0, r15, 0

# r2=name VA r3=epid (HOBJ name ≤15 chars)
app_svc_reg:
    addi r1, r0, 33
    ecall
    jalr r0, r15, 0

# r2=name VA → r1=epid
app_svc_lookup:
    addi r1, r0, 34
    ecall
    jalr r0, r15, 0
