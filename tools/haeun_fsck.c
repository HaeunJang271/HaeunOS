#include "haeun/hfs.h"

#include <stdio.h>
#include <stdlib.h>

/** Repairs a dirty or inconsistent HFS disk image in place. */
int main(int argc, char **argv) {
    FILE *file;
    long length;
    uint8_t *image;
    uint32_t fixes = 0;
    size_t n;
    if (argc != 2) {
        fprintf(stderr, "usage: haeun-fsck <disk.img>\n");
        return 2;
    }
    file = fopen(argv[1], "rb+");
    if (file == NULL) return 1;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }
    image = (uint8_t *)malloc((size_t)length);
    if (image == NULL) {
        fclose(file);
        return 1;
    }
    n = fread(image, 1u, (size_t)length, file);
    if (n != (size_t)length ||
        haeun_hfs_fsck(image, (size_t)length, &fixes) != HAEUN_HFS_OK) {
        free(image);
        fclose(file);
        return 1;
    }
    if (fseek(file, 0, SEEK_SET) != 0 ||
        fwrite(image, 1u, (size_t)length, file) != (size_t)length ||
        fclose(file) != 0) {
        free(image);
        return 1;
    }
    free(image);
    printf("fsck complete, fixes=%u\n", fixes);
    return 0;
}
