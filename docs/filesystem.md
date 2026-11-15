# Filesystem (Release 1.0)

## HFS v1

Host library + `haeun-mkfs` / `haeun-fsck`. Apps mode mounts an in-RAM HFS image for path OPEN.

## Process bridge

Global FD table (`HAEUN_PROC_NFD = 4`) with `owner_pid`; EXIT/fault close-by-owner.  
Console device id `1`. Path OPEN via host assist when HFS mounted.

M20.7 guest assists (root basename only): `FS_READDIR` 47, `FS_UNLINK` 48, `FS_STAT` 49, `FS_SEEK` 50 — see [m20-file-manager-report.md](m20-file-manager-report.md).

Details: [hfs-v1.md](hfs-v1.md), [process-hfs-v1.md](process-hfs-v1.md), [process-hfs-v2.md](process-hfs-v2.md).
