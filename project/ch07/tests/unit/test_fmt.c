/* test_fmt.c - tests for lib/fmt.c, run on the host.
 *
 * Each check formats into a buffer with fmt_snprintf and compares with
 * the string the C library's printf would produce for the same call.
 */
#include <stdint.h>

#include "test.h"
#include "fmt.h"

/* Format into a static buffer and return it. */
static const char *f(const char *fmt, ...)
{
    static char buf[128];
    va_list ap;
    va_start(ap, fmt);
    fmt_vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

void test_fmt_divmod64(void)
{
    uint32_t r;
    CHECK(fmt_divmod64(0, 10, &r) == 0);
    CHECK_INT(r, 0);
    CHECK(fmt_divmod64(12345, 10, &r) == 1234);
    CHECK_INT(r, 5);
    CHECK(fmt_divmod64(UINT64_MAX, 10, &r) == UINT64_MAX / 10);
    CHECK_INT(r, UINT64_MAX % 10);
    CHECK(fmt_divmod64(0x123456789ABCDEFull, 16, &r) == 0x123456789ABCDEull);
    CHECK_INT(r, 0xF);
    CHECK(fmt_divmod64(UINT64_MAX, 65536, &r) == UINT64_MAX >> 16);
    CHECK_INT(r, 0xFFFF);
}

void test_fmt_integers(void)
{
    CHECK_STR(f("%d", 0), "0");
    CHECK_STR(f("%d %i", -42, 42), "-42 42");
    CHECK_STR(f("%d", INT32_MIN), "-2147483648");
    CHECK_STR(f("%u", 4294967295u), "4294967295");
    CHECK_STR(f("%x %X", 0xbeefu, 0xbeefu), "beef BEEF");
    CHECK_STR(f("%ld", -7L), "-7");
    CHECK_STR(f("100%%"), "100%");
    CHECK_STR(f("%q"), "%q");                   /* unknown: printed as is */
}

void test_fmt_width_and_flags(void)
{
    CHECK_STR(f("[%5d]", 42), "[   42]");
    CHECK_STR(f("[%-5d]", 42), "[42   ]");
    CHECK_STR(f("[%05d]", 42), "[00042]");
    CHECK_STR(f("[%05d]", -42), "[-0042]");     /* zeros go after the sign */
    CHECK_STR(f("[%08x]", 0x1f), "[0000001f]");
    CHECK_STR(f("[%*d]", 4, 7), "[   7]");
    CHECK_STR(f("[%2d]", 12345), "[12345]");    /* width is a minimum */
}

void test_fmt_64bit(void)
{
    CHECK_STR(f("%llu", 18446744073709551615ull), "18446744073709551615");
    CHECK_STR(f("%lld", INT64_MIN), "-9223372036854775808");
    CHECK_STR(f("%llx", 0x123456789abcdefull), "123456789abcdef");
    CHECK_STR(f("%016llx", 0x100000ull), "0000000000100000");
}

void test_fmt_strings_and_chars(void)
{
    CHECK_STR(f("%s, %s", "hello", "world"), "hello, world");
    CHECK_STR(f("[%8s]", "ab"), "[      ab]");
    CHECK_STR(f("[%-8s]", "ab"), "[ab      ]");
    CHECK_STR(f("%s", (char *)NULL), "(null)");
    CHECK_STR(f("%c%c%c", 'o', 'k', '!'), "ok!");
    /* %p: 0x and every digit of a pointer (8 in the kernel, 16 here). */
    CHECK_STR(f("%p", (void *)0x1234),
              sizeof(void *) == 4 ? "0x00001234" : "0x0000000000001234");
}

void test_fmt_snprintf_truncates(void)
{
    char buf[6];
    CHECK_INT(fmt_snprintf(buf, sizeof buf, "%s", "sprout!"), 7);
    CHECK_STR(buf, "sprou");
    CHECK_INT(fmt_snprintf(buf, sizeof buf, "%d", 12), 2);
    CHECK_STR(buf, "12");
    buf[0] = 'z';
    CHECK_INT(fmt_snprintf(buf, 0, "%d", 12), 2);  /* size 0: no write */
    CHECK(buf[0] == 'z');
}
