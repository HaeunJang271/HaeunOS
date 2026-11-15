#include "haeun/disk_persist.h"

#include <stdio.h>

/**
 * Writes the full disk buffer back to path (persistent virtual disk image).
 */
int haeun_disk_write_back(const char *path, const uint8_t *disk, size_t size) {
    FILE *file;
    size_t wrote;
    if (path == NULL || disk == NULL || size == 0u || (size % 512u) != 0u)
        return -1;
    file = fopen(path, "wb");
    if (file == NULL) return -1;
    wrote = fwrite(disk, 1u, size, file);
    if (fclose(file) != 0) return -1;
    return wrote == size ? 0 : -1;
}
