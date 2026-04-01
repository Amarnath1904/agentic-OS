#ifndef KERNEL_FS_RAMFS_RAMFS_H
#define KERNEL_FS_RAMFS_RAMFS_H

#include "../../kernel.h"
#include "path.h"

#define RAMFS_MAX_ENTRIES 96

typedef enum {
    RAMFS_NODE_FILE = 0,
    RAMFS_NODE_DIR = 1,
} ramfs_type_t;

typedef void (*ramfs_list_emit_fn)(const char* name, void* ctx);

void ramfs_init(void);
i32 ramfs_mkdir(const char* abs_path);
i32 ramfs_create_empty_file(const char* abs_path);
i32 ramfs_write_file(const char* abs_path, const void* buf, u32 len);
i32 ramfs_read_file(const char* abs_path, void* buf, u32 cap, u32 offset, u32* out_len);
i32 ramfs_unlink(const char* abs_path);
i32 ramfs_list(const char* abs_dir, ramfs_list_emit_fn emit, void* ctx);
/* Root "/" is a directory even without an entry */
i32 ramfs_get_node_type(const char* abs_path, ramfs_type_t* out_type);

#endif
