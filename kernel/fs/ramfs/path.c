/* ============================================
 * Path helpers for ramfs (bounded, no alloc)
 * ============================================ */

#include "path.h"
#include "../../string/kstring.h"

static char* trim_left(const char* s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return (char*)s;
}

static void trim_in_place(char* s, size_t* len_out)
{
    if (s == NULL || *s == '\0') {
        *len_out = 0;
        return;
    }
    char* start = trim_left(s);
    if (start != s) {
        size_t len = kstrlen(start) + 1;
        kmemcpy(s, start, len);
    }
    size_t n = kstrlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
        n--;
    }
    s[n] = '\0';
    *len_out = n;
}

static i32 segment_invalid(const char* seg, size_t len)
{
    if (len == 0) {
        return 0;
    }
    if (len == 1 && seg[0] == '.') {
        return 1;
    }
    if (len == 2 && seg[0] == '.' && seg[1] == '.') {
        return 1;
    }
    if (len >= RAMFS_MAX_NAME) {
        return 1;
    }
    return 0;
}

i32 path_normalize(char* inout, size_t cap)
{
    if (inout == NULL || cap < 2) {
        return STATUS_INVALID;
    }

    size_t raw_len = 0;
    trim_in_place(inout, &raw_len);
    if (raw_len == 0) {
        inout[0] = '/';
        inout[1] = '\0';
        return STATUS_OK;
    }

    char tmp[RAMFS_MAX_PATH];
    char out[RAMFS_MAX_PATH];
    if (raw_len >= sizeof(tmp)) {
        return STATUS_INVALID;
    }
    kmemcpy(tmp, inout, raw_len + 1);

    if (tmp[0] != '/') {
        return STATUS_INVALID;
    }

    size_t o = 0;
    size_t i = 1;
    while (i < raw_len) {
        while (i < raw_len && tmp[i] == '/') {
            i++;
        }
        if (i >= raw_len) {
            break;
        }
        size_t start = i;
        while (i < raw_len && tmp[i] != '/') {
            if (tmp[i] == '\0') {
                return STATUS_INVALID;
            }
            i++;
        }
        size_t seg_len = i - start;
        if (segment_invalid(tmp + start, seg_len)) {
            return STATUS_INVALID;
        }
        if (o + 1 + seg_len >= sizeof(out) || o + 1 + seg_len >= cap) {
            return STATUS_INVALID;
        }
        out[o++] = '/';
        kmemcpy(out + o, tmp + start, seg_len);
        o += seg_len;
    }
    out[o] = '\0';
    if (o == 0) {
        out[0] = '/';
        out[1] = '\0';
    }
    if (kstrlen(out) >= cap) {
        return STATUS_INVALID;
    }
    kmemcpy(inout, out, kstrlen(out) + 1);
    return STATUS_OK;
}

i32 path_join(char* out, size_t cap, const char* cwd, const char* rel)
{
    if (out == NULL || cwd == NULL || rel == NULL || cap < 2) {
        return STATUS_INVALID;
    }

    const char* r = trim_left(rel);
    if (r[0] == '/') {
        size_t rl = kstrlen(r);
        if (rl + 1 >= cap || rl >= RAMFS_MAX_PATH) {
            return STATUS_INVALID;
        }
        kmemcpy(out, r, rl + 1);
        return path_normalize(out, cap);
    }

    size_t cw = kstrlen(cwd);
    if (cw >= cap) {
        return STATUS_INVALID;
    }

    kmemcpy(out, cwd, cw + 1);
    size_t olen = cw;
    if (cw == 1 && cwd[0] == '/') {
        /* already ends with slash semantics */
    } else {
        if (olen + 2 >= cap) {
            return STATUS_INVALID;
        }
        out[olen++] = '/';
        out[olen] = '\0';
    }
    size_t rl = kstrlen(r);
    if (olen + rl + 1 >= cap || rl >= RAMFS_MAX_PATH) {
        return STATUS_INVALID;
    }
    kmemcpy(out + olen, r, rl + 1);
    return path_normalize(out, cap);
}

i32 path_parent(const char* path, char* parent_out, size_t cap)
{
    if (path == NULL || parent_out == NULL || cap < 2) {
        return STATUS_INVALID;
    }
    if (kstrcmp(path, "/") == 0) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
        return STATUS_OK;
    }
    size_t len = kstrlen(path);
    if (len >= RAMFS_MAX_PATH) {
        return STATUS_INVALID;
    }
    size_t i = len;
    while (i > 0 && path[i] != '/') {
        i--;
    }
    if (path[i] != '/') {
        return STATUS_INVALID;
    }
    if (i == 0) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
        return STATUS_OK;
    }
    if (i + 1 >= cap) {
        return STATUS_INVALID;
    }
    kmemcpy(parent_out, path, i);
    parent_out[i] = '\0';
    return STATUS_OK;
}

i32 path_basename(const char* path, char* base_out, size_t cap)
{
    if (path == NULL || base_out == NULL || cap < 2) {
        return STATUS_INVALID;
    }
    if (kstrcmp(path, "/") == 0) {
        base_out[0] = '/';
        base_out[1] = '\0';
        return STATUS_OK;
    }
    size_t len = kstrlen(path);
    if (len == 0 || len >= RAMFS_MAX_PATH) {
        return STATUS_INVALID;
    }
    size_t i = len;
    while (i > 0 && path[i - 1] != '/') {
        i--;
    }
    const char* base = path + i;
    size_t blen = kstrlen(base);
    if (blen + 1 >= cap || blen >= RAMFS_MAX_NAME) {
        return STATUS_INVALID;
    }
    kmemcpy(base_out, base, blen + 1);
    return STATUS_OK;
}
