/* ============================================
 * RAM-backed hierarchical filesystem (kernel only)
 * ============================================ */

#include "ramfs.h"
#include "../../memory/memory.h"
#include "../../string/kstring.h"

typedef struct {
    bool used;
    ramfs_type_t type;
    char path[RAMFS_MAX_PATH];
    u8* data;
    u32 size;
} ramfs_entry_t;

static ramfs_entry_t g_entries[RAMFS_MAX_ENTRIES];

u32 ramfs_free_entries(void)
{
    u32 count = 0;
    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) if (!g_entries[i].used) count++;
    return count;
}

static void entry_clear(ramfs_entry_t* e)
{
    if (e->data != NULL) {
        kfree(e->data);
    }
    kmemset(e, 0, sizeof(*e));
}

void ramfs_init(void)
{
    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) {
        if (g_entries[i].used && g_entries[i].data != NULL) {
            kfree(g_entries[i].data);
        }
        kmemset(&g_entries[i], 0, sizeof(g_entries[i]));
    }
}

static i32 find_entry(const char* abs_path, u32* out_idx)
{
    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) {
        if (!g_entries[i].used) {
            continue;
        }
        if (kstrcmp(g_entries[i].path, abs_path) == 0) {
            if (out_idx != NULL) {
                *out_idx = i;
            }
            return STATUS_OK;
        }
    }
    return STATUS_NOT_FOUND;
}

static ramfs_entry_t* alloc_entry(void)
{
    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) {
        if (!g_entries[i].used) {
            g_entries[i].used = true;
            g_entries[i].type = RAMFS_NODE_FILE;
            g_entries[i].path[0] = '\0';
            g_entries[i].data = NULL;
            g_entries[i].size = 0;
            return &g_entries[i];
        }
    }
    return NULL;
}

static void release_entry(u32 idx)
{
    if (idx >= RAMFS_MAX_ENTRIES) {
        return;
    }
    entry_clear(&g_entries[idx]);
}

static i32 path_copy_normalize(const char* abs_path, char* out, size_t cap)
{
    if (abs_path == NULL) {
        return STATUS_INVALID;
    }
    size_t len = kstrlen(abs_path);
    if (len + 1 >= cap || len >= RAMFS_MAX_PATH) {
        return STATUS_INVALID;
    }
    kmemcpy(out, abs_path, len + 1);
    return path_normalize(out, cap);
}

static i32 ensure_dir_at(const char* abs_dir_path)
{
    u32 idx;
    if (find_entry(abs_dir_path, &idx) == STATUS_OK) {
        if (g_entries[idx].type != RAMFS_NODE_DIR) {
            return STATUS_INVALID;
        }
        return STATUS_OK;
    }
    ramfs_entry_t* e = alloc_entry();
    if (e == NULL) {
        return STATUS_OUT_OF_MEM;
    }
    e->type = RAMFS_NODE_DIR;
    kmemcpy(e->path, abs_dir_path, kstrlen(abs_dir_path) + 1);
    return STATUS_OK;
}

/* Ensure all path prefixes (except possibly final) exist as dirs. */
static i32 ensure_dir_chain_for_parent(const char* abs_parent)
{
    if (kstrcmp(abs_parent, "/") == 0) {
        return STATUS_OK;
    }
    char walk[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_parent, walk, sizeof(walk));
    if (st != STATUS_OK) {
        return st;
    }

    size_t n = kstrlen(walk);
    size_t p = 1;
    char cur[RAMFS_MAX_PATH];
    cur[0] = '/';
    cur[1] = '\0';

    while (p < n) {
        size_t start = p;
        while (p < n && walk[p] != '/') {
            p++;
        }
        size_t slen = p - start;

        size_t cl = kstrlen(cur);
        if (cl == 1 && cur[0] == '/') {
            if (1 + slen + 1 > sizeof(cur)) {
                return STATUS_INVALID;
            }
            kmemcpy(cur + 1, walk + start, slen);
            cur[1 + slen] = '\0';
        } else {
            if (cl + 1 + slen + 1 > sizeof(cur)) {
                return STATUS_INVALID;
            }
            cur[cl] = '/';
            kmemcpy(cur + cl + 1, walk + start, slen);
            cur[cl + 1 + slen] = '\0';
        }

        st = ensure_dir_at(cur);
        if (st != STATUS_OK) {
            return st;
        }
        if (p < n && walk[p] == '/') {
            p++;
        }
    }
    return STATUS_OK;
}

i32 ramfs_mkdir(const char* abs_path)
{
    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }
    if (kstrcmp(norm, "/") == 0) {
        return STATUS_INVALID;
    }

    u32 idx;
    if (find_entry(norm, &idx) == STATUS_OK) {
        if (g_entries[idx].type == RAMFS_NODE_DIR) {
            return STATUS_OK;
        }
        return STATUS_INVALID;
    }

    char parent[RAMFS_MAX_PATH];
    st = path_parent(norm, parent, sizeof(parent));
    if (st != STATUS_OK) {
        return st;
    }
    st = ensure_dir_chain_for_parent(parent);
    if (st != STATUS_OK) {
        return st;
    }

    ramfs_entry_t* e = alloc_entry();
    if (e == NULL) {
        return STATUS_OUT_OF_MEM;
    }
    e->type = RAMFS_NODE_DIR;
    kmemcpy(e->path, norm, kstrlen(norm) + 1);
    return STATUS_OK;
}

i32 ramfs_create_empty_file(const char* abs_path)
{
    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }
    if (kstrcmp(norm, "/") == 0) {
        return STATUS_INVALID;
    }

    u32 idx;
    if (find_entry(norm, &idx) == STATUS_OK) {
        return STATUS_INVALID;
    }

    char parent[RAMFS_MAX_PATH];
    st = path_parent(norm, parent, sizeof(parent));
    if (st != STATUS_OK) {
        return st;
    }
    st = ensure_dir_chain_for_parent(parent);
    if (st != STATUS_OK) {
        return st;
    }

    ramfs_entry_t* e = alloc_entry();
    if (e == NULL) {
        return STATUS_OUT_OF_MEM;
    }
    e->type = RAMFS_NODE_FILE;
    e->size = 0;
    e->data = NULL;
    kmemcpy(e->path, norm, kstrlen(norm) + 1);
    return STATUS_OK;
}

i32 ramfs_write_file(const char* abs_path, const void* buf, u32 len)
{
    if (buf == NULL && len > 0) {
        return STATUS_INVALID;
    }

    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }
    if (kstrcmp(norm, "/") == 0) {
        return STATUS_INVALID;
    }

    char parent[RAMFS_MAX_PATH];
    st = path_parent(norm, parent, sizeof(parent));
    if (st != STATUS_OK) {
        return st;
    }
    st = ensure_dir_chain_for_parent(parent);
    if (st != STATUS_OK) {
        return st;
    }

    u32 idx;
    if (find_entry(norm, &idx) == STATUS_OK) {
        if (g_entries[idx].type == RAMFS_NODE_DIR) {
            return STATUS_INVALID;
        }
        u8* new_data = NULL;
        if (len > 0) {
            new_data = (u8*)kmalloc(len, MEM_ZERO);
            if (new_data == NULL) {
                return STATUS_OUT_OF_MEM;
            }
            kmemcpy(new_data, buf, len);
        }
        if (g_entries[idx].data != NULL) {
            kfree(g_entries[idx].data);
        }
        g_entries[idx].data = new_data;
        g_entries[idx].size = len;
        return STATUS_OK;
    }

    ramfs_entry_t* e = alloc_entry();
    if (e == NULL) {
        return STATUS_OUT_OF_MEM;
    }
    u32 ei = (u32)(e - g_entries);
    e->type = RAMFS_NODE_FILE;
    e->size = len;
    if (len == 0) {
        e->data = NULL;
    } else {
        e->data = (u8*)kmalloc(len, MEM_ZERO);
        if (e->data == NULL) {
            release_entry(ei);
            return STATUS_OUT_OF_MEM;
        }
        kmemcpy(e->data, buf, len);
    }
    kmemcpy(e->path, norm, kstrlen(norm) + 1);
    return STATUS_OK;
}

i32 ramfs_read_file(const char* abs_path, void* buf, u32 cap, u32 offset, u32* out_len)
{
    if (out_len != NULL) {
        *out_len = 0;
    }
    if (buf == NULL && cap > 0) {
        return STATUS_INVALID;
    }

    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }

    u32 idx;
    if (find_entry(norm, &idx) != STATUS_OK) {
        return STATUS_NOT_FOUND;
    }
    if (g_entries[idx].type != RAMFS_NODE_FILE) {
        return STATUS_INVALID;
    }

    u32 n = g_entries[idx].size;
    if (offset >= n) {
        return STATUS_OK;
    }
    u32 avail = n - offset;
    u32 give = (cap < avail) ? cap : avail;
    if (give > 0 && g_entries[idx].data != NULL) {
        kmemcpy(buf, g_entries[idx].data + offset, give);
    }
    if (out_len != NULL) {
        *out_len = give;
    }
    return STATUS_OK;
}

static bool dir_has_any_child(const char* abs_dir)
{
    char prefix[RAMFS_MAX_PATH];
    size_t plen = kstrlen(abs_dir);
    if (plen + 2 > sizeof(prefix)) {
        return true;
    }
    kmemcpy(prefix, abs_dir, plen + 1);
    if (!(plen == 1 && abs_dir[0] == '/')) {
        prefix[plen] = '/';
        prefix[plen + 1] = '\0';
        plen++;
    }

    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) {
        if (!g_entries[i].used) {
            continue;
        }
        const char* p = g_entries[i].path;
        if (kstrcmp(p, abs_dir) == 0) {
            continue;
        }
        if (plen == 1 && prefix[0] == '/') {
            /* root: child paths like /foo */
            if (p[0] == '/' && kstrlen(p) > 1) {
                const char* rest = p + 1;
                if (kstrchr(rest, '/') == NULL) {
                    return true;
                }
            }
            continue;
        }
        if (kstrncmp(p, prefix, plen) == 0 && kstrlen(p) > plen) {
            return true;
        }
    }
    return false;
}

i32 ramfs_unlink(const char* abs_path)
{
    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }
    if (kstrcmp(norm, "/") == 0) {
        return STATUS_INVALID;
    }

    u32 idx;
    if (find_entry(norm, &idx) != STATUS_OK) {
        return STATUS_NOT_FOUND;
    }

    if (g_entries[idx].type == RAMFS_NODE_DIR) {
        if (dir_has_any_child(norm)) {
            return STATUS_INVALID;
        }
        release_entry(idx);
        return STATUS_OK;
    }

    release_entry(idx);
    return STATUS_OK;
}

static void prefix_for_dir(const char* abs_dir, char* prefix_out, size_t cap, size_t* plen_out)
{
    size_t plen = kstrlen(abs_dir);
    if (plen == 1 && abs_dir[0] == '/') {
        prefix_out[0] = '/';
        prefix_out[1] = '\0';
        *plen_out = 1;
        return;
    }
    if (plen + 2 > cap) {
        *plen_out = 0;
        prefix_out[0] = '\0';
        return;
    }
    kmemcpy(prefix_out, abs_dir, plen + 1);
    prefix_out[plen] = '/';
    prefix_out[plen + 1] = '\0';
    *plen_out = plen + 1;
}

i32 ramfs_list(const char* abs_dir, ramfs_list_emit_fn emit, void* ctx)
{
    if (emit == NULL) {
        return STATUS_INVALID;
    }

    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_dir, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }

    if (kstrcmp(norm, "/") != 0) {
        u32 idx;
        if (find_entry(norm, &idx) != STATUS_OK) {
            return STATUS_NOT_FOUND;
        }
        if (g_entries[idx].type != RAMFS_NODE_DIR) {
            return STATUS_INVALID;
        }
    }

    char prefix[RAMFS_MAX_PATH];
    size_t plen = 0;
    prefix_for_dir(norm, prefix, sizeof(prefix), &plen);
    if (plen == 0 && kstrcmp(norm, "/") != 0) {
        return STATUS_INVALID;
    }

    for (u32 i = 0; i < RAMFS_MAX_ENTRIES; i++) {
        if (!g_entries[i].used) {
            continue;
        }
        const char* p = g_entries[i].path;
        if (plen == 1 && prefix[0] == '/') {
            if (p[0] != '/' || kstrlen(p) <= 1) {
                continue;
            }
            const char* rest = p + 1;
            const char* slash = kstrchr(rest, '/');
            if (slash != NULL) {
                continue;
            }
            emit(rest, ctx);
            continue;
        }
        if (kstrncmp(p, prefix, plen) != 0 || kstrlen(p) <= plen) {
            continue;
        }
        const char* rest = p + plen;
        if (kstrchr(rest, '/') != NULL) {
            continue;
        }
        emit(rest, ctx);
    }
    return STATUS_OK;
}

i32 ramfs_get_node_type(const char* abs_path, ramfs_type_t* out_type)
{
    if (out_type == NULL) {
        return STATUS_INVALID;
    }

    char norm[RAMFS_MAX_PATH];
    i32 st = path_copy_normalize(abs_path, norm, sizeof(norm));
    if (st != STATUS_OK) {
        return st;
    }

    if (kstrcmp(norm, "/") == 0) {
        *out_type = RAMFS_NODE_DIR;
        return STATUS_OK;
    }

    u32 idx;
    if (find_entry(norm, &idx) != STATUS_OK) {
        return STATUS_NOT_FOUND;
    }
    *out_type = g_entries[idx].type;
    return STATUS_OK;
}
