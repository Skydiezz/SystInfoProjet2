#include "lib_tar.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#define TAR_CHECKSUM_SIZE 8
#define TAR_MAGIC_OFFSET 257
#define TAR_VERSION_OFFSET 263
#define TAR_CHECKSUM_OFFSET 148
#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_TYPEFLAG_OFFSET 156
#define TAR_LINKNAME_OFFSET 157

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path){

    uint8_t buffer[TAR_BLOCK_SIZE];

    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        return 0;
    }

    while (read(tar_fd, buffer, TAR_BLOCK_SIZE) == TAR_BLOCK_SIZE) {

        int is_null_block = 1;
        for (int i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (buffer[i] != 0) {
                is_null_block = 0;
                break;
            }
        }
        if (is_null_block) {
            break;
        }

        char name[TAR_NAME_SIZE + 1];
        memcpy(name, buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE] = '\0';

        if (strcmp(name, path) == 0) {
            return 1;
        }
    }
    return 0;


}






// Function to calculate the checksum to make sure the checksum in the header is correct

int calculate_checksum(const uint8_t *header) {
    int checksum = 0;
    for (int i = 0; i < TAR_BLOCK_SIZE; i++) {
        if (i >= TAR_CHECKSUM_OFFSET && i < TAR_CHECKSUM_OFFSET + TAR_CHECKSUM_SIZE) {
            checksum += 32;
        } else {
            checksum += header[i];
        }
    }
    return checksum;
}

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {

    uint8_t buffer[TAR_BLOCK_SIZE];
    int header_count = 0;

    while (read(tar_fd, buffer, TAR_BLOCK_SIZE) == TAR_BLOCK_SIZE) {
        
        int is_null_block = 1;
        for (int i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (buffer[i] != 0) {
                is_null_block = 0;
                break;
            }
        }
        if (is_null_block) {
            break;
        }

        // Vérifie la valeur magique
        if (strncmp((char *)(buffer + TAR_MAGIC_OFFSET), TMAGIC, TMAGLEN) != 0) {
            return -1;
        }

        // Vérifie la version
        if (strncmp((char *)(buffer + TAR_VERSION_OFFSET), "00", TVERSLEN) != 0) {
            return -2;
        }

        // Vérifie le checksum
        char stored_checksum_str[TAR_CHECKSUM_SIZE + 1];
        memcpy(stored_checksum_str, buffer + TAR_CHECKSUM_OFFSET, TAR_CHECKSUM_SIZE);
        stored_checksum_str[TAR_CHECKSUM_SIZE] = '\0';
        int stored_checksum = (int)strtol(stored_checksum_str, NULL, 8);

        int computed_checksum = calculate_checksum(buffer);
        if (stored_checksum != computed_checksum) {
            return -3;
        }
        header_count++;
    }

    return header_count;

}

// Just a function to regroup "is_dir", "is_file", "is_symlink" because they are very similar
int is_smth(int tar_fd, char *path, char type){

    uint8_t buffer[TAR_BLOCK_SIZE];

    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        return 0;
    }

    while (read(tar_fd, buffer, TAR_BLOCK_SIZE) == TAR_BLOCK_SIZE) {

        int is_null_block = 1;
        for (int i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (buffer[i] != 0) {
                is_null_block = 0;
                break;
            }
        }
        if (is_null_block) {
            break;
        }

        char name[TAR_NAME_SIZE + 1];
        memcpy(name, buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE] = '\0';

        if (strcmp(name, path) == 0) {
            // Verify if it's the right type
            char typeflag = buffer[TAR_TYPEFLAG_OFFSET];
            if (typeflag == type) {
                return 1;
            }
            return 0;
        }
    }
    return 0;
} 


/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    return is_smth(tar_fd, path, (char) DIRTYPE);
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    return is_smth(tar_fd, path, (char) REGTYPE);
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
    return is_smth(tar_fd, path, (char) SYMTYPE);
}


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    uint8_t buffer[TAR_BLOCK_SIZE];
    size_t entries_found = 0;

    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        return 0;
    }

    size_t path_len = strlen(path);
    while (read(tar_fd, buffer, TAR_BLOCK_SIZE) == TAR_BLOCK_SIZE) {
        // Check for null block
        int is_null_block = 1;
        for (int i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (buffer[i] != 0) {
                is_null_block = 0;
                break;
            }
        }
        if (is_null_block) {
            break;
        }

        char name[TAR_NAME_SIZE + 2];
        memcpy(name, buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE-1] = '/';
        name[TAR_NAME_SIZE] = '\0';

        // Check if the entry matches the given path
        if (strncmp(name, path, path_len) == 0) {
            if (strncmp(name, path, path_len) == 0 && strlen(name)-2 > path_len) {
                if (entries_found < *no_entries) {
                    // Allocate memory for the entry
                    entries[entries_found] = malloc(TAR_NAME_SIZE + 1);
                    if (entries[entries_found] == NULL) {
                        fprintf(stderr, "Memory allocation failed\n");
                        // Clean up already allocated entries
                        for (size_t i = 0; i < entries_found; i++) {
                            free(entries[i]);
                        }
                        return 0;
                    }

                    if (buffer[TAR_TYPEFLAG_OFFSET] == SYMTYPE) {
                        char linkname[TAR_NAME_SIZE + 1];
                        memcpy(linkname, buffer + TAR_LINKNAME_OFFSET, TAR_NAME_SIZE);
                        linkname[TAR_NAME_SIZE] = '\0';
                        strncpy(entries[entries_found], linkname, TAR_NAME_SIZE);
                        entries[entries_found][TAR_NAME_SIZE] = '\0';
                    } else {
                        strncpy(entries[entries_found], name, TAR_NAME_SIZE);
                        entries[entries_found][TAR_NAME_SIZE] = '\0';
                    }
                }
                entries_found++;
            }
        }
    }

    *no_entries = entries_found;
    return entries_found > 0 ? 1 : 0;
}
/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */

static size_t octal_s(const char *octal){
    size_t size = 0;
    sscanf(octal, "%zo", &size);
    return size;

}


ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    
    if (tar_fd < 0 || !path || !dest || !len || *len == 0){
        return -1;
    }

    if(lseek(tar_fd, 0, SEEK_SET) == 1){
        return -1;
    }

    uint8_t buffer[TAR_BLOCK_SIZE];
    size_t file_size;

    while(read(tar_fd, buffer, TAR_BLOCK_SIZE) == TAR_BLOCK_SIZE){
        int is_null = 1;
        for(int i = 0; i < TAR_BLOCK_SIZE; i++){
            if(buffer[i] != 0){
                is_null = 0;
                break;
            }
        }
        if(is_null){
            break;
        }

        char name[TAR_NAME_SIZE + 1];
        memcpy(name,buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE] = '\0';

        char flag = buffer[TAR_TYPEFLAG_OFFSET];
        if(strcmp(name, path) == 0){
            if(flag == SYMTYPE){
                char link[TAR_NAME_SIZE + 1];
                memcpy(link, buffer+TAR_LINKNAME_OFFSET, TAR_NAME_SIZE);
                link[TAR_NAME_SIZE] = '\0';
                return read_file(tar_fd, link, offset, dest, len);
            } 
        } else if (flag != REGTYPE){
            return -1;
        }

        file_size = octal_s((char *)(buffer + 124));
        if(offset >= file_size){
            return -2;
        }

        size_t bytes_r = *len;
        if(offset + bytes_r > file_size){
            bytes_r = file_size - offset;
        }

        size_t blocks_skip = offset / TAR_BLOCK_SIZE;
        size_t offset_block = offset % TAR_BLOCK_SIZE;

        if(lseek(tar_fd, blocks_skip * TAR_BLOCK_SIZE, SEEK_CUR) == -1){
            return -1;
        }

        size_t rest = bytes_r;
        size_t total_r = 0;

        while(rest > 0){
            uint8_t data_block[TAR_BLOCK_SIZE];
            if(read(tar_fd, data_block, TAR_BLOCK_SIZE)!= TAR_BLOCK_SIZE){
                return -1;
            }

            size_t copy = TAR_BLOCK_SIZE - offset_block;
            if(copy > rest){
                copy = rest;
            }

            memcpy(dest + total_r, data_block + offset_block, copy);
            total_r += copy;
            rest -= copy;

            offset_block = 0;
        }

        *len = bytes_r;

        if(offset + bytes_r == file_size){
            return 0;
        }

        return file_size - (offset + bytes_r);



        lseek(tar_fd, offset, SEEK_CUR);

        if(bytes_r < 0){
            return -1;
        }
        
        *len = bytes_r;

        if(offset + bytes_r == file_size){
            return 0;
        }

        return file_size - (offset + bytes_r);
    }

    file_size = octal_s((char*)(buffer + 124));
    size_t blocks_skip = (file_size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
    if(lseek(tar_fd, blocks_skip * TAR_BLOCK_SIZE, SEEK_CUR) == -1){
        return -1;
    }
    return -1;
}