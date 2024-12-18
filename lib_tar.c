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

        if (strncmp((char *)(buffer + TAR_MAGIC_OFFSET), "", TMAGLEN)==0){
            continue;
        }


        // Vérifie la valeur magique
        if (strncmp((char *)(buffer + TAR_MAGIC_OFFSET), TMAGIC, TMAGLEN) != 0){
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

int check_for_list(char *name, size_t pathlen){
    int check = 0;
    for(int i=pathlen+1; name[i] != '\0'; i++){
        if(name[i]=='/'){
            check += 1;
        }
    }
    return check;


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
    size_t path_len = strlen(path);



 
    char *adjusted_path = NULL;
    // add a '/' in the end of the path if it's not yet done 
    if (path[path_len - 1] != '/' && is_symlink(tar_fd, path) != 1) {
  
        adjusted_path = malloc(path_len + 2);
        if (adjusted_path == NULL) {
            return 0; 
        }


        strcpy(adjusted_path, path);
        adjusted_path[path_len] = '/';
        adjusted_path[path_len + 1] = '\0';
        path_len += 1;
    } else {
       
        adjusted_path = strdup(path);
        if (adjusted_path == NULL) {
            return 0; 
        }
    }


    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        return 0;
    }

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

        char name[TAR_NAME_SIZE + 1];
        memcpy(name, buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE] = '\0';

        // Check if the entry matches the given path
        if (strncmp(name, adjusted_path, path_len) == 0) {
          
                if(strlen(name) == path_len){
                    
                    if(buffer[TAR_TYPEFLAG_OFFSET] == SYMTYPE){
    
                        char linkname[TAR_NAME_SIZE + 1];
                        memcpy(linkname, buffer + TAR_LINKNAME_OFFSET, TAR_NAME_SIZE);
                        linkname[TAR_NAME_SIZE] = '\0';
                        free(adjusted_path);
                        return list(tar_fd, linkname, entries, no_entries);
                    } else {
                        continue;
                    }
                } else {
                    if((check_for_list(name, path_len) == 1 && buffer[TAR_TYPEFLAG_OFFSET] == DIRTYPE) || check_for_list(name, path_len) == 0){
                        if (entries_found < *no_entries) {
                        // Allocate memory for the entry
                            entries[entries_found] = malloc(TAR_NAME_SIZE + 1);
                            if (entries[entries_found] == NULL) {
                                fprintf(stderr, "Memory allocation failed\n");
                                for (size_t i = 0; i < entries_found; i++) {
                                    free(entries[i]);
                                }
                                return 0;
                            }


                            strncpy(entries[entries_found], name, TAR_NAME_SIZE);
                            entries[entries_found][TAR_NAME_SIZE] = '\0';

                        }
                        entries_found++;
                    }

                }

        }
    }

    *no_entries = entries_found;
    free(adjusted_path);
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

ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    uint8_t buffer[TAR_BLOCK_SIZE];
    size_t file_size = 0;
    size_t blocks_to_skip = 0;
    
    // Go back to the beginning of the tar
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        return -1;
    }

    // First, find the file and get its size
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

        // Check if this is a symlink that needs resolution
        if (buffer[TAR_TYPEFLAG_OFFSET] == SYMTYPE) {
            char linkname[TAR_NAME_SIZE + 1];
            memcpy(linkname, buffer + TAR_LINKNAME_OFFSET, TAR_NAME_SIZE);
            linkname[TAR_NAME_SIZE] = '\0';
            
            // Recursively resolve symlink
            return read_file(tar_fd, linkname, offset, dest, len);
        }

        char name[TAR_NAME_SIZE + 1];
        memcpy(name, buffer, TAR_NAME_SIZE);
        name[TAR_NAME_SIZE] = '\0';

        if (strcmp(name, path) == 0 && buffer[TAR_TYPEFLAG_OFFSET] == REGTYPE) {
            // Extract file size (in octal)
            char size_str[12];
            memcpy(size_str, buffer + 124, 11);
            size_str[11] = '\0';
            file_size = (size_t)strtol(size_str, NULL, 8);

            // Check if offset is valid
            if (offset >= file_size) {
                return -2;
            }

            // Calculate how many blocks to skip to get to the offset
            blocks_to_skip = offset / TAR_BLOCK_SIZE;
            size_t block_offset = offset % TAR_BLOCK_SIZE;

            // Skip to the correct data block
            for (size_t i = 0; i < blocks_to_skip; i++) {
                if (read(tar_fd, buffer, TAR_BLOCK_SIZE) != TAR_BLOCK_SIZE) {
                    return -1;
                }
            }

            // Read the block containing the offset
            if (read(tar_fd, buffer, TAR_BLOCK_SIZE) != TAR_BLOCK_SIZE) {
                return -1;
            }

            // Calculate how many bytes we can read
            size_t bytes_to_read = (file_size - offset < *len) ? file_size - offset : *len;
            
            // Adjust for block offset
            size_t start_index = block_offset;
            size_t copy_size = TAR_BLOCK_SIZE - start_index < bytes_to_read ? 
                               TAR_BLOCK_SIZE - start_index : bytes_to_read;
            
            // Copy data to destination
            memcpy(dest, buffer + start_index, copy_size);
            
            size_t bytes_read = copy_size;

            // If we need to read more, continue reading blocks
            while (bytes_read < bytes_to_read) {
                if (read(tar_fd, buffer, TAR_BLOCK_SIZE) != TAR_BLOCK_SIZE) {
                    break;
                }

                size_t remaining = bytes_to_read - bytes_read;
                size_t to_copy = remaining < TAR_BLOCK_SIZE ? remaining : TAR_BLOCK_SIZE;
                
                memcpy(dest + bytes_read, buffer, to_copy);
                bytes_read += to_copy;
            }

            // Set the number of bytes actually read
            *len = bytes_read;

            // Return remaining bytes (if any)
            return file_size - (offset + bytes_read) > 0 ? 
                   file_size - (offset + bytes_read) : 0;
        }
    }

    // File not found
    return -1;
}