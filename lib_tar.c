#include "lib_tar.h"
#define TAR_CHECKSUM_SIZE 8
#define TAR_MAGIC_OFFSET 257
#define TAR_VERSION_OFFSET 263
#define TAR_CHECKSUM_OFFSET 148
#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_TYPEFLAG_OFFSET 156

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
        if (strncmp((char *)(buffer + TAR_VERSION_OFFSET), TVERSION, TVERSLEN) != 0) {
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
int is_smth(int tar_fd, char *path, char *type){

        uint8_t buffer[TAR_BLOCK_SIZE];

    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("lseek");
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
    return is_smth(tar_fd, path, DIRTYPE);
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
    return is_smth(tar_fd, path, REGTYPE);
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
    return is_smth(tar_fd, path, SYMTYPE);
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
    return 0;
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
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    return 0;
}