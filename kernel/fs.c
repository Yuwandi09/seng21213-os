/* =============================================================================
 * SENG21213-OS :: In-Memory RAM-Disk File System Implementation
 * File   : kernel/fs.c
 * ============================================================================*/
#include "fs.h"

static fs_file_t files[FS_MAX_FILES];

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static size_t k_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].used = false;
        files[i].size = 0;
        files[i].name[0] = '\0';
    }

    /* Pre-populate a greeting readme.txt */
    fs_create("readme.txt");
    const char *msg = "Welcome to SENG21213-OS!\nStage 4 RAM-disk file system active.\n";
    fs_write("readme.txt", msg, k_strlen(msg));

    fs_create("info.txt");
    const char *info = "Course: SENG 21213\nUniversity of Kelaniya\n";
    fs_write("info.txt", info, k_strlen(info));
}

int fs_create(const char *name) {
    if (!name || !*name) return -1;

    /* Check if file already exists */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && k_strcmp(files[i].name, name) == 0) {
            return -2; /* Already exists */
        }
    }

    /* Find free slot */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = true;
            files[i].size = 0;
            k_strncpy(files[i].name, name, FS_MAX_NAME_LEN);
            files[i].data[0] = '\0';
            return 0;
        }
    }

    return -3; /* Disk full */
}

int fs_write(const char *name, const char *data, size_t len) {
    if (!name || !data) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && k_strcmp(files[i].name, name) == 0) {
            if (len >= FS_MAX_FILE_SIZE) {
                len = FS_MAX_FILE_SIZE - 1;
            }
            for (size_t j = 0; j < len; j++) {
                files[i].data[j] = data[j];
            }
            files[i].data[len] = '\0';
            files[i].size = (uint32_t)len;
            return (int)len;
        }
    }

    return -1; /* Not found */
}

int fs_read(const char *name, char *buf, size_t max_len) {
    if (!name || !buf || max_len == 0) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && k_strcmp(files[i].name, name) == 0) {
            size_t copy_len = files[i].size;
            if (copy_len >= max_len) {
                copy_len = max_len - 1;
            }
            for (size_t j = 0; j < copy_len; j++) {
                buf[j] = files[i].data[j];
            }
            buf[copy_len] = '\0';
            return (int)copy_len;
        }
    }

    return -1; /* Not found */
}

int fs_delete(const char *name) {
    if (!name || !*name) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && k_strcmp(files[i].name, name) == 0) {
            files[i].used = false;
            files[i].size = 0;
            files[i].name[0] = '\0';
            return 0;
        }
    }

    return -1; /* Not found */
}

fs_file_t *fs_get_files(void) {
    return files;
}

int fs_file_count(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used) count++;
    }
    return count;
}
