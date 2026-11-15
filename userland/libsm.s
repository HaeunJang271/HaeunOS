# M20.6 Service Manager guest wrappers (link with apps that need SERVICE_*).
.isa 4
.global app_sm_count
.global app_sm_info
.global app_sm_start
.global app_sm_stop
.global app_sm_status
.global app_sm_restart

app_sm_count:
    addi r1, r0, 41
    ecall
    jalr r0, r15, 0

# r2=id r3=field r4=buf r5=cap
app_sm_info:
    addi r1, r0, 42
    ecall
    jalr r0, r15, 0

# r2=id → r1=pid
app_sm_start:
    addi r1, r0, 43
    ecall
    jalr r0, r15, 0

# r2=id
app_sm_stop:
    addi r1, r0, 44
    ecall
    jalr r0, r15, 0

# r2=id → r1=0; r2=state r3=pid r4=exit|runs r5=fails|restarts
app_sm_status:
    addi r1, r0, 45
    ecall
    jalr r0, r15, 0

# r2=id → r1=pid
app_sm_restart:
    addi r1, r0, 46
    ecall
    jalr r0, r15, 0
