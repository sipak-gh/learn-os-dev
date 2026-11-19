/* fmt.h — printf-style formatting without a C library.
 *
 * fmt_format() does the work and hands each output character to a
 * callback, so the same code prints to the screen (kprintf), into a buffer
 * (fmt_snprintf) or, in the unit tests, into a host program's buffer.
 *
 * Supported: %d %i %u %x %X %p %s %c %%, the flags '-' (left-justify) and
 * '0' (pad with zeros), a field width (digits or '*'), and the length
 * modifiers l and ll (long, long long). No floating point.
 */
#ifndef SPROUT_FMT_H
#define SPROUT_FMT_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fmt_out_fn)(char c, void *ctx);

/* Format into out(); returns the number of characters produced. */
int fmt_format(fmt_out_fn out, void *ctx, const char *fmt, va_list ap);

/* Like C's vsnprintf/snprintf: writes at most size - 1 characters and a
 * '\0' (if size > 0); returns the length the whole result would have. */
int fmt_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int fmt_snprintf(char *buf, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* 64-bit division by a small number using only 32-bit divisions: on i386
 * GCC compiles a 64-bit '/' into a call to libgcc's __udivdi3, which a
 * freestanding kernel doesn't have. Returns n / d; *rem = n % d. d must be
 * at most 65536. */
uint64_t fmt_divmod64(uint64_t n, uint32_t d, uint32_t *rem);

#endif
