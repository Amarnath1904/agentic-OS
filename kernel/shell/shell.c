/* ============================================
 * Minimal interactive shell (ramfs + line input)
 * ============================================ */

#include "shell.h"
#include "../kernel.h"
#include "../drivers/vga.h"
#include "../io/line_input.h"
#include "../fs/ramfs/ramfs.h"
#include "../fs/ramfs/path.h"
#include "../string/kstring.h"
#include "../net/ipv4.h"
#include "../net/tcp_probe.h"
#include "../net/net_config.h"
#include "../net/ai.h"
#include "../io.h"
#include "nono.h"

#define SHELL_LINE_CAP 512
#define SHELL_CMD_CAP 32

static char s_cwd[RAMFS_MAX_PATH] = "/";

static const char* skip_leading_ws(const char* s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

/* Copy first word from s into out; return pointer to remainder (first char after word, may be ws). */
static const char* take_word(const char* s, char* out, size_t cap)
{
    s = skip_leading_ws(s);
    size_t i = 0;
    while (s[i] != '\0' && s[i] != ' ' && s[i] != '\t') {
        if (i + 1 >= cap) {
            out[0] = '\0';
            return s + i;
        }
        out[i] = s[i];
        i++;
    }
    out[i] = '\0';
    return s + i;
}

static void print_err(const char* msg)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print(msg);
    vga_print("\n");
    vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);
}

static void resolve_path(char* abs_out, size_t cap, const char* user_path)
{
    i32 st = path_join(abs_out, cap, s_cwd, user_path);
    if (st != STATUS_OK) {
        abs_out[0] = '\0';
    }
}

static void cmd_help(void)
{
    vga_print("Commands: help pwd cd mkdir touch cat write rm ls tcpping ask ai nono shutdown\n");
    vga_print("ask <question> - ask Gemini; ai - enter chat (/exit to return).\n");
    vga_print("shutdown - power off the OS (RAM files will be lost).\n");
    vga_print("nono <task> - create folders or write text files in the current directory.\n");
}

static void cmd_shutdown(void)
{
    vga_print("Shutting down. RAM filesystem contents will be lost.\n");
    /* Bochs i440FX BIOS maps PIIX4 PM control to 0xb004; S5 type is 0.
     * This target-specific shutdown does not power off the host computer. */
    asm volatile("cli");
    outw(0xb004, 0x2000);
    vga_print("Power-off unavailable. System halted; close the emulator window.\n");
    halt();
}

static void cmd_ask(const char* args)
{
    static char reply[AI_REPLY_CAP];
    args = skip_leading_ws(args);
    if (!*args) { print_err("Usage: ask <question>"); return; }
    vga_print("Thinking...\n");
    bool remote_error;
    const char* error = ai_ask(args, reply, &remote_error);
    if (error) { print_err(error); return; }
    if (remote_error) { print_err(reply); return; }
    vga_print("AI: ");
    u32 rows = 0, col = 4;
    for (u32 i = 0; reply[i]; i++) {
        vga_putchar(reply[i]);
        if (reply[i] == '\n') { rows++; col = 0; }
        else if (++col >= 80) { rows++; col = 0; }
        if (rows >= 18 && reply[i + 1]) {
            char pause[2];
            vga_print("\n[Enter for more]");
            line_input_read(pause, sizeof(pause), true);
            vga_print("\n"); rows = 0; col = 0;
        }
    }
    vga_print("\n");
}

static void cmd_ai(void)
{
    char question[AI_PROMPT_CAP];
    vga_print("Gemini questions. Each question is independent. /exit returns to shell.\n");
    for (;;) {
        vga_print("You: ");
        line_input_read(question, sizeof(question), true);
        vga_print("\n");
        if (kstrcmp(question, "/exit") == 0) return;
        if (*skip_leading_ws(question)) cmd_ask(question);
    }
}

static void cmd_pwd(void)
{
    vga_print(s_cwd);
    vga_print("\n");
}

static void cmd_cd(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    take_word(args, rel, sizeof(rel));
    if (rel[0] == '\0') {
        print_err("cd: missing path");
        return;
    }
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("cd: invalid path");
        return;
    }
    ramfs_type_t t;
    i32 st = ramfs_get_node_type(abs, &t);
    if (st != STATUS_OK || t != RAMFS_NODE_DIR) {
        print_err("cd: not a directory");
        return;
    }
    kmemcpy(s_cwd, abs, kstrlen(abs) + 1);
}

static void cmd_mkdir(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    take_word(args, rel, sizeof(rel));
    if (rel[0] == '\0') {
        print_err("mkdir: missing path");
        return;
    }
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("mkdir: invalid path");
        return;
    }
    i32 st = ramfs_mkdir(abs);
    if (st != STATUS_OK) {
        print_err("mkdir: failed");
    }
}

static void cmd_touch(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    take_word(args, rel, sizeof(rel));
    if (rel[0] == '\0') {
        print_err("touch: missing path");
        return;
    }
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("touch: invalid path");
        return;
    }
    i32 st = ramfs_create_empty_file(abs);
    if (st != STATUS_OK) {
        print_err("touch: failed");
    }
}

static void cmd_cat(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    take_word(args, rel, sizeof(rel));
    if (rel[0] == '\0') {
        print_err("cat: missing path");
        return;
    }
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("cat: invalid path");
        return;
    }

    char chunk[256];
    u32 offset = 0;
    for (;;) {
        u32 got = 0;
        i32 st = ramfs_read_file(abs, chunk, sizeof(chunk) - 1, offset, &got);
        if (st != STATUS_OK) {
            print_err("cat: cannot read file");
            return;
        }
        if (got == 0) {
            break;
        }
        chunk[got] = '\0';
        vga_print(chunk);
        offset += got;
    }
    vga_print("\n");
}

static void cmd_write(const char* args)
{
    args = skip_leading_ws(args);
    if (*args == '\0') {
        print_err("write: missing path");
        return;
    }
    const char* sp = args;
    while (*sp != '\0' && *sp != ' ' && *sp != '\t') {
        sp++;
    }
    if (*args == '\0') {
        print_err("write: missing path");
        return;
    }
    size_t plen = (size_t)(sp - args);
    if (plen + 1 >= RAMFS_MAX_PATH) {
        print_err("write: path too long");
        return;
    }
    char rel[RAMFS_MAX_PATH];
    kmemcpy(rel, args, plen);
    rel[plen] = '\0';

    const char* content = skip_leading_ws(sp);
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("write: invalid path");
        return;
    }

    u32 len = (u32)kstrlen(content);
    i32 st = ramfs_write_file(abs, content, len);
    if (st != STATUS_OK) {
        print_err("write: failed");
    }
}

static void cmd_rm(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    take_word(args, rel, sizeof(rel));
    if (rel[0] == '\0') {
        print_err("rm: missing path");
        return;
    }
    char abs[RAMFS_MAX_PATH];
    resolve_path(abs, sizeof(abs), rel);
    if (abs[0] == '\0') {
        print_err("rm: invalid path");
        return;
    }
    i32 st = ramfs_unlink(abs);
    if (st != STATUS_OK) {
        print_err("rm: failed");
    }
}

static void list_emit(const char* name, void* ctx)
{
    (void)ctx;
    vga_print(name);
    vga_print("\n");
}

static u16 parse_u16_dec(const char* s)
{
    u16 v = 0;
    if (s == NULL || *s == '\0') {
        return 0;
    }
    while (*s >= '0' && *s <= '9') {
        v = (u16)(v * 10u + (u16)(*s - '0'));
        s++;
        if (v > 60000u) {
            return 0;
        }
    }
    return v;
}

static void cmd_tcpping(const char* args)
{
    char ipbuf[64];
    char portbuf[32];
    const char* r;
    u32 dip;
    u16 port;

    r = take_word(args, ipbuf, sizeof(ipbuf));
    if (ipbuf[0] == '\0') {
        print_err("tcpping: usage: tcpping <ipv4> [port]");
        return;
    }
    if (!ipv4_parse_dotted(ipbuf, &dip)) {
        print_err("tcpping: bad ipv4");
        return;
    }
    take_word(r, portbuf, sizeof(portbuf));
    if (portbuf[0] == '\0') {
        port = (u16)TCP_PROBE_DEFAULT_PORT;
    } else {
        port = parse_u16_dec(portbuf);
        if (port == 0u) {
            print_err("tcpping: bad port");
            return;
        }
    }
    vga_print("tcpping ");
    vga_print(ipbuf);
    vga_print(" ...\n");
    if (!tcp_probe_host_port(dip, port)) {
        print_err("tcpping: no reply (check net_config.h + Bochs ne2k)");
        return;
    }
    vga_print("tcpping: ok (SYN-ACK)\n");
}

static void cmd_ls(const char* args)
{
    char rel[RAMFS_MAX_PATH];
    const char* rest = take_word(args, rel, sizeof(rel));
    (void)rest;
    const char* list_path = s_cwd;
    char abs[RAMFS_MAX_PATH];
    if (rel[0] != '\0') {
        resolve_path(abs, sizeof(abs), rel);
        if (abs[0] == '\0') {
            print_err("ls: invalid path");
            return;
        }
        list_path = abs;
    }
    i32 st = ramfs_list(list_path, list_emit, NULL);
    if (st != STATUS_OK) {
        print_err("ls: failed");
    }
}

static void dispatch_line(char* line)
{
    char cmd[SHELL_CMD_CAP];
    const char* rest = take_word(line, cmd, sizeof(cmd));
    if (cmd[0] == '\0') {
        return;
    }

    if (kstrcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (kstrcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (kstrcmp(cmd, "cd") == 0) {
        cmd_cd(rest);
    } else if (kstrcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(rest);
    } else if (kstrcmp(cmd, "touch") == 0) {
        cmd_touch(rest);
    } else if (kstrcmp(cmd, "cat") == 0) {
        cmd_cat(rest);
    } else if (kstrcmp(cmd, "write") == 0) {
        cmd_write(skip_leading_ws(rest));
    } else if (kstrcmp(cmd, "rm") == 0) {
        cmd_rm(rest);
    } else if (kstrcmp(cmd, "ls") == 0) {
        cmd_ls(rest);
    } else if (kstrcmp(cmd, "tcpping") == 0) {
        cmd_tcpping(rest);
    } else if (kstrcmp(cmd, "ask") == 0) {
        cmd_ask(rest);
    } else if (kstrcmp(cmd, "ai") == 0) {
        cmd_ai();
    } else if (kstrcmp(cmd, "shutdown") == 0) {
        cmd_shutdown();
    } else if (kstrcmp(cmd, "nono") == 0) {
        nono_run(skip_leading_ws(rest), s_cwd);
    } else {
        print_err("unknown command (try help)");
    }
}

void shell_run(void)
{
    s_cwd[0] = '/';
    s_cwd[1] = '\0';
    vga_print("Ask Gemini: ask <question>, or type ai for chat. Type help for commands.\n");

    for (;;) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print("agentic:");
        vga_print(s_cwd);
        vga_print("$ ");
        vga_set_color(VGA_COLOR_LIGHT_GRAY, VGA_COLOR_BLACK);

        char line[SHELL_LINE_CAP];
        line_input_read(line, sizeof(line), true);
        vga_print("\n");
        dispatch_line(line);
    }
}
