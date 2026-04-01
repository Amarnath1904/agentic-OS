#ifndef KERNEL_STRING_KSTRING_H
#define KERNEL_STRING_KSTRING_H

#include <stddef.h>

size_t kstrlen(const char* s);
int kstrcmp(const char* a, const char* b);
int kstrncmp(const char* a, const char* b, size_t n);
void* kmemcpy(void* dest, const void* src, size_t n);
void* kmemset(void* dest, int c, size_t n);
char* kstrchr(const char* s, int c);

#endif
