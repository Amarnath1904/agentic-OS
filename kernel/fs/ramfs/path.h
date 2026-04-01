#ifndef KERNEL_FS_RAMFS_PATH_H
#define KERNEL_FS_RAMFS_PATH_H

#include "../../kernel.h"
#include <stddef.h>

#define RAMFS_MAX_PATH 128
#define RAMFS_MAX_NAME 64

/* Normalize in place: leading slash, collapse //, trim edges; reject .. and . */
i32 path_normalize(char* inout, size_t cap);

/* Absolute path in out; if rel starts with /, copy and normalize; else cwd/rel */
i32 path_join(char* out, size_t cap, const char* cwd, const char* rel);

/* Parent directory path; "/" parent is "/" */
i32 path_parent(const char* path, char* parent_out, size_t cap);

/* Final path component (basename); "/" -> "/" */
i32 path_basename(const char* path, char* base_out, size_t cap);

#endif
