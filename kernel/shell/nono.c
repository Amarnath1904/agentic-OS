/* Model output is data: parse/validate the entire bounded plan before mutation.
 * The only callable actions are RAMFS mkdir/write; never shell dispatch. */
#include "nono.h"
#include "../net/ai.h"
#include "../fs/ramfs/ramfs.h"
#include "../string/kstring.h"
#include "../drivers/vga.h"

typedef struct {
    char path[RAMFS_MAX_PATH];
    char* content;
    u16 size;
    char op;
} action_t;
static action_t actions[64];
static char reply[AI_REPLY_CAP];

static void number(u32 n)
{
    char buf[12]; u32 i = 0;
    do { buf[i++] = '0' + n % 10; n /= 10; } while (n);
    while (i) vga_putchar(buf[--i]);
}
static int hex(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}
static bool valid_relative(const char* p)
{
    if (!*p || *p == '/' || kstrlen(p) > 110) return false;
    const char* start = p;
    for (;;) {
        char c = *p;
        if (c == '/' || !c) {
            u32 len = p - start;
            if (!len || len > 63 || (len == 1 && *start == '.')
                || (len == 2 && start[0] == '.' && start[1] == '.')) return false;
            if (!c) return true;
            start = p + 1;
        } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
        p++;
    }
}
static bool parse(const char* cwd, u32* count)
{
    *count = 0;
    if (kstrncmp(reply, "NPLAN1\n", 7)) return false;
    char* p = reply + 7;
    while (*p) {
        if (kstrcmp(p, "END") == 0) return *count > 0;
        if (*count >= 64 || (p[0] != 'D' && p[0] != 'W') || p[1] != ' ') return false;
        char* end = kstrchr(p, '\n');
        if (!end) return false;
        *end = 0;
        action_t* a = &actions[*count];
        a->op = *p; a->size = 0; a->content = NULL;
        char* path = p + 2;
        if (a->op == 'W') {
            char* space = kstrchr(path, ' ');
            if (!space) return false;
            *space = 0;
            char* content = space + 1;
            a->content = content;
            if (kstrcmp(content, "-") != 0) {
                u32 len = kstrlen(content);
                if (!len || len > 1024 || len % 2) return false;
                for (u32 i = 0; i < len; i += 2) {
                    int hi = hex(content[i]), lo = hex(content[i + 1]);
                    if (hi < 0 || lo < 0) return false;
                    char c = (hi << 4) | lo;
                    if ((c < 32 && c != '\n' && c != '\r' && c != '\t') || (u8)c > 126) return false;
                    content[a->size++] = c;
                }
            }
        }
        if (!valid_relative(path) || path_join(a->path, sizeof(a->path), cwd, path) != STATUS_OK) return false;
        for (u32 i = 0; i < *count; i++) if (!kstrcmp(a->path, actions[i].path)) return false;
        (*count)++;
        p = end + 1;
    }
    return false;
}
static bool preflight(u32 count)
{
    u32 needed = 0;
    for (u32 i = 0; i < count; i++) {
        action_t* a = &actions[i];
        ramfs_type_t type;
        i32 status = ramfs_get_node_type(a->path, &type);
        if (status == STATUS_OK) {
            if ((a->op == 'D') != (type == RAMFS_NODE_DIR)) return false;
        } else if (status == STATUS_NOT_FOUND) needed++;
        else return false;
        char parent[RAMFS_MAX_PATH];
        if (path_parent(a->path, parent, sizeof(parent)) != STATUS_OK) return false;
        bool exists = ramfs_get_node_type(parent, &type) == STATUS_OK && type == RAMFS_NODE_DIR;
        for (u32 j = 0; j < i && !exists; j++)
            if (actions[j].op == 'D' && !kstrcmp(actions[j].path, parent)) exists = true;
        if (!exists) return false;
    }
    return needed <= ramfs_free_entries();
}
static bool verify(action_t* a)
{
    ramfs_type_t type;
    if (ramfs_get_node_type(a->path, &type) != STATUS_OK) return false;
    if (a->op == 'D') return type == RAMFS_NODE_DIR;
    char content[513]; u32 got = 0;
    if (type != RAMFS_NODE_FILE || ramfs_read_file(a->path, content, sizeof(content), 0, &got) != STATUS_OK
        || got != a->size) return false;
    for (u32 i = 0; i < got; i++) if (content[i] != a->content[i]) return false;
    return true;
}
void nono_run(const char* task, const char* cwd)
{
    if (!*task) { vga_print("Usage: nono <create folders or write files task>\n"); return; }
    vga_print("Nono: planning your task...\n");
    bool remote_error;
    const char* error = ai_plan(task, reply, &remote_error);
    if (error || remote_error) { vga_print(error ? error : reply); vga_print("\nNo files changed.\n"); return; }
    u32 count;
    if (!parse(cwd, &count) || !preflight(count)) {
        vga_print("Nono: invalid plan, conflicting paths, or insufficient space. No files changed.\n"); return;
    }
    u32 dirs = 0, files = 0;
    for (u32 i = 0; i < count; i++) {
        action_t* a = &actions[i];
        i32 status = a->op == 'D' ? ramfs_mkdir(a->path) : ramfs_write_file(a->path, a->content, a->size);
        if (status != STATUS_OK || !verify(a)) {
            vga_print("Nono stopped at "); vga_print(a->path);
            vga_print(". Earlier completed actions remain.\n"); return;
        }
        if (a->op == 'D') dirs++; else files++;
    }
    vga_print("Nono: done. Verified "); number(dirs); vga_print(" folders and ");
    number(files); vga_print(" files under "); vga_print(cwd); vga_print(".\n");
    vga_print("First path: "); vga_print(actions[0].path);
    vga_print("\nUse ls/cat to inspect. RAM files disappear on shutdown.\n");
}
