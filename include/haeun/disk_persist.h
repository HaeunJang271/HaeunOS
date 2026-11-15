#ifndef HAEUN_DISK_PERSIST_H
#define HAEUN_DISK_PERSIST_H

#include <stddef.h>
#include <stdint.h>

/**
 * Atomically replaces path with the current disk buffer contents.
 * Returns 0 on success.
 */
int haeun_disk_write_back(const char *path, const uint8_t *disk, size_t size);

#endif
