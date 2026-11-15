# HAEUN File System v1 (HFS)

Status: **on-disk format, host codec/mkfs/fsck, guest block/VFS path, and persistent disk write-back are implemented and tested**.

## Goals

- Store files and directories on the virtual block device (512-byte LBAs).
- Survive full VM shutdown/restart via a host disk image file that is written back after guest block writes.
- Support permission bits, create/read/write/unlink/mkdir, and dirty-bit recovery through `fsck`.

## Disk layout

Default image size is 256 blocks (128 KiB). All multi-byte fields are little-endian.

| Region | Location |
|---|---|
| Superblock | LBA 0 |
| Allocation bitmap | LBA 1 |
| Inode table | LBA `inode_start` .. `data_start-1` |
| Data blocks | LBA `data_start` .. `block_count-1` |

### Superblock (512 bytes)

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic `HFS1` |
| 4 | 2 | version `1` |
| 6 | 2 | block_size `512` |
| 8 | 4 | block_count |
| 12 | 4 | inode_count |
| 16 | 4 | root_inum (`1`) |
| 20 | 4 | bitmap_lba (`1`) |
| 24 | 4 | inode_start |
| 28 | 4 | data_start |
| 32 | 4 | generation |
| 36 | 4 | state (`0` clean, `1` dirty) |
| 40 | 472 | reserved zero |

### Inode (64 bytes)

| Offset | Size | Field |
|---:|---:|---|
| 0 | 2 | mode (`HF_IFREG`/`HF_IFDIR` \| `0o777` perms) |
| 2 | 2 | nlink |
| 4 | 4 | size_bytes |
| 8 | 48 | `direct[12]` data LBAs (0 = unused) |
| 56 | 2 | uid |
| 58 | 2 | gid |
| 60 | 4 | mtime (guest tick or tool counter) |

Inode 0 is unused. Inode 1 is the root directory.

### Directory entry (32 bytes)

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | inum (`0` = free slot) |
| 4 | 28 | name, NUL-padded, max 27 chars |

## Permissions

`mode & 7` = other, `>>3 & 7` = group, `>>6 & 7` = user (R=4,W=2,X=1). Directory search needs X; file read needs R; write/create/unlink needs W on the directory.

## Persistence

`haeun-vm … --disk path` loads the image into a buffer attached to the block device. Guest `COMMAND=write` mutates that buffer. On VM exit the CLI writes the buffer back to `path`. Files are not claimed persistent if they exist only in RAM without write-back.

## Recovery

Mount sets `state=dirty`. Clean unmount sets `state=clean`. If mount finds `dirty`, `haeun_hfs_fsck` rebuilds the data bitmap from live inodes, clears invalid direct pointers, and bumps `generation`.

## Guest interface

Guest VFS issues block MMIO reads/writes only. Host HFS helpers create images and verify contents in tests; they are not a substitute for guest I/O during guest execution.
