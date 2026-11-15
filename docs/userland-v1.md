# HAEUN userland v1

Status: **init + shell command surface, tiny libc stubs, and HFS-backed file commands are specified; guest shell image and host mkfs/fsck tools are in-tree**.

## Layout

| Path | Role |
|---|---|
| `userland/init.s` | Init process entry; launches the shell |
| `userland/shell.s` | HAEUN shell command dispatcher |
| `userland/libc.s` | putc/puts/strcmp-style helpers |
| `userland/cmds_*.s` | Command implementations |
| `tools/haeun_mkfs` | Format persistent HFS images |
| `tools/haeun_fsck` | Dirty-volume recovery |

## Shell commands

`help`, `clear`, `version`, `uname`, `cpuinfo`, `meminfo`, `ls`, `cd`, `pwd`, `mkdir`, `touch`, `cat`, `rm`, `cp`, `mv`, `ps`, `kill`, `mount`, `df`.

Informational commands print from the guest shell. `mount`/`df`/`fsdemo` exercise the guest block driver against a real HFS image (magic check, dirty bit, persistent stamp). Full create/read/write/unlink/mkdir/readdir/permission checks are implemented in the host HFS library (`haeun_hfs_*`) and verified by `hfs-tests`; the guest shell lists the file-command surface and stamps the volume so VM write-back persistence is observable.

## Persistence workflow

```text
haeun-mkfs build/root.img 256 64
haeun-vm build/shell.hxf --disk build/root.img
haeun-fsck build/root.img
```

After the guest halts, `haeun-vm` writes `root.img` back to disk so a subsequent mount sees prior creates/writes.
