#include "haeun/hfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Creates a persistent HFS disk image file. */
int main(int argc, char **argv) {
    const char *path;
    size_t blocks = 256u;
    uint32_t inodes = 64u;
    size_t bytes;
    uint8_t *image;
    FILE *file;
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: haeun-mkfs <disk.img> [blocks] [inodes]\n");
        return 2;
    }
    path = argv[1];
    if (argc >= 3) blocks = (size_t)strtoul(argv[2], NULL, 0);
    if (argc >= 4) inodes = (uint32_t)strtoul(argv[3], NULL, 0);
    if (blocks < 16u) return 2;
    bytes = blocks * HAEUN_HFS_BLOCK_SIZE;
    image = (uint8_t *)calloc(1, bytes);
    if (image == NULL) return 2;
    if (haeun_hfs_mkfs(image, bytes, inodes) != HAEUN_HFS_OK) {
        free(image);
        return 1;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        free(image);
        return 1;
    }
    if (fwrite(image, 1u, bytes, file) != bytes || fclose(file) != 0) {
        free(image);
        return 1;
    }
    free(image);
    printf("formatted HFS image %s (%zu blocks)\n", path, blocks);
    return 0;
}
