# HAEUN shell: runs a fixed command script demonstrating the command set.
.isa 4
.global shell_main
.global cmd_help
.global cmd_version
.global cmd_uname
.global cmd_cpuinfo
.global cmd_meminfo
.global cmd_clear
.global cmd_pwd
.global cmd_ps
.global cmd_df
.global cmd_mount
.global cmd_ls
.global cmd_fsdemo

shell_main:
    addi r3, r15, 0
    jal r1, p_ret
    .word 0x00009800
p_ret:
    lw r1, 0(r1)
    sw r3, 0(r1)
    jal r15, cmd_help
    jal r15, cmd_version
    jal r15, cmd_uname
    jal r15, cmd_cpuinfo
    jal r15, cmd_meminfo
    jal r15, cmd_clear
    jal r15, cmd_pwd
    jal r15, cmd_mount
    jal r15, cmd_df
    jal r15, cmd_ls
    jal r15, cmd_fsdemo
    jal r15, cmd_ps
    jal r1, p_ret2
    .word 0x00009800
p_ret2:
    lw r1, 0(r1)
    lw r2, 0(r1)
    addi r1, r0, 0
    jalr r0, r2, 0
