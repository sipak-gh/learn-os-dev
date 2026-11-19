/* string.h — the few <string.h> functions the kernel needs.
 *
 * A freestanding C implementation has no C library, but GCC may still emit
 * calls to memcpy, memmove, memset and memcmp (for struct copies or loops
 * it recognises), so a kernel must define those four. The rest are here
 * because we want them. Same names and contracts as standard C.
 */
#ifndef SPROUT_STRING_H
#define SPROUT_STRING_H

#include <stddef.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

#endif
