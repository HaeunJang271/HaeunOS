# M20.7 HFS helpers (FS_READDIR/UNLINK/STAT/SEEK). Link with filemgr apps.
.isa 4
.global app_fs_readdir
.global app_fs_unlink
.global app_fs_stat
.global app_fs_seek

# r2=buf VA r3=cap → r1=bytes
app_fs_readdir:
    addi r1, r0, 47
    ecall
    jalr r0, r15, 0

# r2=path VA → r1=0/-1
app_fs_unlink:
    addi r1, r0, 48
    ecall
    jalr r0, r15, 0

# r2=path VA → r1=0; r2=size r3=mode
app_fs_stat:
    addi r1, r0, 49
    ecall
    jalr r0, r15, 0

# r2=fd r3=offset → r1=0/-1
app_fs_seek:
    addi r1, r0, 50
    ecall
    jalr r0, r15, 0
