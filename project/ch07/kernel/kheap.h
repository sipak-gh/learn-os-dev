/* kheap.h — kmalloc and kfree: the kernel's heap. */
#ifndef SPROUT_KHEAP_H
#define SPROUT_KHEAP_H

#include <stdbool.h>
#include <stddef.h>

#define KHEAP_START 0xD0000000u     /* virtual; nothing else lives there */
#define KHEAP_MAX   (64u << 20)     /* at most 64 MiB */

void kheap_init(void);

/* n bytes, 8-byte aligned, contents undefined; NULL if out of memory. */
void *kmalloc(size_t n);

/* Free a block from kmalloc; NULL is ignored. Panics on anything else. */
void kfree(void *p);

/* kfree, but return false instead of panicking on a bad pointer. */
bool kheap_free(void *p);

/* Print every block and the totals. */
void kheap_print(void);

#endif
