/* =============================================================================
 * SENG21213-OS :: In-Memory RAM-Disk File System Header
 * File   : kernel/fs.h
 * Stage 4: Simple flat in-memory file system with touch, write, cat, rm, ls.
 * ============================================================================*/
#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAX_FILES      16
#define FS_MAX_NAME_LEN   32
#define FS_MAX_FILE_SIZE  1024

typedef struct {
    char     name[FS_MAX_NAME_LEN];
    uint32_t size;
    char     data[FS_MAX_FILE_SIZE];
    bool     used;
} fs_file_t;

void       fs_init(void);
int        fs_create(const char *name);
int        fs_write(const char *name, const char *data, size_t len);
int        fs_read(const char *name, char *buf, size_t max_len);
int        fs_delete(const char *name);
fs_file_t *fs_get_files(void);
int        fs_file_count(void);

#endif /* FS_H */
