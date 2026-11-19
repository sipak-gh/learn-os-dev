/* fmt.c — the printf engine. See fmt.h. */
#include "fmt.h"

#include <stdbool.h>

#include "string.h"

/* Long division in base 65536: divide the 64-bit number 16 bits at a
 * time, carrying the remainder into the next step, exactly like dividing
 * by hand digit by digit. With d <= 65536 every intermediate value
 * (rem << 16 | next 16 bits) fits in 32 bits. */
uint64_t fmt_divmod64(uint64_t n, uint32_t d, uint32_t *rem)
{
    uint64_t q = 0;
    uint32_t r = 0;
    for (int shift = 48; shift >= 0; shift -= 16) {
        uint32_t part = (r << 16) | (uint32_t)((n >> shift) & 0xFFFF);
        q |= (uint64_t)(part / d) << shift;
        r = part % d;
    }
    if (rem)
        *rem = r;
    return q;
}

/* One conversion's settings, from the flags and width before it. */
struct spec {
    bool left;          /* '-': pad on the right */
    bool zero;          /* '0': pad with '0' instead of ' ' */
    int width;          /* minimum field width */
};

struct sink {
    fmt_out_fn out;
    void *ctx;
    int count;
};

static void emit(struct sink *s, char c)
{
    s->out(c, s->ctx);
    s->count++;
}

static void emit_padding(struct sink *s, char c, int n)
{
    while (n-- > 0)
        emit(s, c);
}

/* Emit text (len bytes) in a field: sign/prefix first, then zero padding
 * between prefix and digits, or space padding outside everything. */
static void emit_field(struct sink *s, const struct spec *sp,
                       const char *prefix, const char *text, int len)
{
    int plen = (int)strlen(prefix);
    int pad = sp->width - plen - len;

    if (!sp->left && !sp->zero)
        emit_padding(s, ' ', pad);
    while (*prefix)
        emit(s, *prefix++);
    if (!sp->left && sp->zero)
        emit_padding(s, '0', pad);
    for (int i = 0; i < len; i++)
        emit(s, text[i]);
    if (sp->left)
        emit_padding(s, ' ', pad);
}

/* Digits of v in base 10 or 16, written backwards from the end of buf.
 * Returns a pointer to the first digit. */
static char *utoa(char *end, uint64_t v, unsigned base, bool upper)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char *p = end;
    do {
        uint32_t r;
        v = fmt_divmod64(v, base, &r);
        *--p = digits[r];
    } while (v != 0);
    return p;
}

int fmt_format(fmt_out_fn out, void *ctx, const char *fmt, va_list ap)
{
    struct sink s = { out, ctx, 0 };
    char buf[24];                   /* 2^64 has 20 decimal digits */
    char *end = buf + sizeof buf;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            emit(&s, *fmt);
            continue;
        }

        struct spec sp = { false, false, 0 };
        for (;; fmt++) {                    /* flags */
            if (fmt[1] == '-')
                sp.left = true;
            else if (fmt[1] == '0')
                sp.zero = true;
            else
                break;
        }
        if (fmt[1] == '*') {                /* width */
            sp.width = va_arg(ap, int);
            fmt++;
        } else {
            while (fmt[1] >= '0' && fmt[1] <= '9')
                sp.width = sp.width * 10 + (*++fmt - '0');
        }
        int longs = 0;                      /* length: l, ll */
        while (fmt[1] == 'l') {
            longs++;
            fmt++;
        }

        char conv = *++fmt;
        uint64_t u;
        switch (conv) {
        case 'd':
        case 'i': {
            int64_t v = longs >= 2 ? va_arg(ap, long long)
                      : longs == 1 ? va_arg(ap, long)
                      : va_arg(ap, int);
            /* Negate as unsigned: -INT64_MIN would overflow. */
            u = v < 0 ? -(uint64_t)v : (uint64_t)v;
            char *p = utoa(end, u, 10, false);
            emit_field(&s, &sp, v < 0 ? "-" : "", p, (int)(end - p));
            break;
        }
        case 'u':
        case 'x':
        case 'X':
            u = longs >= 2 ? va_arg(ap, unsigned long long)
              : longs == 1 ? va_arg(ap, unsigned long)
              : va_arg(ap, unsigned int);
            {
                char *p = utoa(end, u, conv == 'u' ? 10 : 16, conv == 'X');
                emit_field(&s, &sp, "", p, (int)(end - p));
            }
            break;
        case 'p': {
            /* Pointers print as 0x and the full width of a pointer. */
            u = (uintptr_t)va_arg(ap, void *);
            char *p = utoa(end, u, 16, false);
            struct spec ptr = { sp.left, true, (int)(2 + 2 * sizeof(void *)) };
            emit_field(&s, &ptr, "0x", p, (int)(end - p));
            break;
        }
        case 's': {
            const char *str = va_arg(ap, const char *);
            if (str == NULL)
                str = "(null)";
            sp.zero = false;
            emit_field(&s, &sp, "", str, (int)strlen(str));
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            sp.zero = false;
            emit_field(&s, &sp, "", &c, 1);
            break;
        }
        case '%':
            emit(&s, '%');
            break;
        case '\0':                          /* lone '%' at the very end */
            return s.count;
        default:                            /* unknown: print it as is */
            emit(&s, '%');
            emit(&s, conv);
            break;
        }
    }
    return s.count;
}

struct bufctx {
    char *buf;
    size_t size;
    size_t len;
};

static void buf_out(char c, void *ctx)
{
    struct bufctx *b = ctx;
    if (b->len + 1 < b->size)
        b->buf[b->len] = c;
    b->len++;
}

int fmt_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    struct bufctx b = { buf, size, 0 };
    int n = fmt_format(buf_out, &b, fmt, ap);
    if (size > 0)
        buf[b.len < size ? b.len : size - 1] = '\0';
    return n;
}

int fmt_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = fmt_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}
