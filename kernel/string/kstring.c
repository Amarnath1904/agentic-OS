/* ============================================
 * Minimal freestanding string helpers
 * ============================================ */

#include "kstring.h"
#include "../kernel.h"

size_t kstrlen(const char* s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int kstrcmp(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return (unsigned char)*a - (unsigned char)*b;
        }
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int kstrncmp(const char* a, const char* b, size_t n)
{
    while (n > 0 && *a != '\0' && *b != '\0') {
        if (*a != *b) {
            return (unsigned char)*a - (unsigned char)*b;
        }
        a++;
        b++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

void* kmemcpy(void* dest, const void* src, size_t n)
{
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* kmemset(void* dest, int c, size_t n)
{
    u8* d = (u8*)dest;
    u8 v = (u8)c;
    for (size_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dest;
}

char* kstrchr(const char* s, int c)
{
    char ch = (char)c;
    while (*s != '\0') {
        if (*s == ch) {
            return (char*)s;
        }
        s++;
    }
    if (ch == '\0') {
        return (char*)s;
    }
    return 0;
}
