#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv){
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    char *path = NULL;
    if (argc >= 3) {
        path = argv[2];
    }

    size_t *nentries = malloc(sizeof(size_t));
    if (nentries == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1; // Handle memory allocation failure
    }
    *nentries = 4;

    char **entries = malloc(501 * sizeof(char *));
    if (entries == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1; // Handle memory allocation failure
    }
    for (int i = 0; i < 501; i++) {
        entries[i] = NULL;
    }

    int ret = check_archive(fd);
    list(fd, path, entries, nentries);

    printf("The list of archive is :");
    for(int i=0; entries[i] != NULL; i++){
        printf("  %s\n", entries[i]);
    }
    printf("check_archive returned %d\n", ret);
    
    for (int i = 0; i < 501; i++) {
        if (entries[i] != NULL) {
            free(entries[i]);
        }
    }
    free(nentries);
    free(entries);
    return 0;
}