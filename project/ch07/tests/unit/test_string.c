/* test_string.c - tests for lib/string.c, run on the host. */
#include "test.h"
#include "string.h"

void test_string_mem(void)
{
    char buf[8];
    CHECK(memset(buf, 'x', sizeof buf) == buf);
    CHECK(memcmp(buf, "xxxxxxxx", 8) == 0);
    CHECK(memcpy(buf + 2, "abc", 3) == buf + 2);
    CHECK(memcmp(buf, "xxabcxxx", 8) == 0);
    CHECK(memcmp("abc", "abd", 3) < 0);
    CHECK(memcmp("abd", "abc", 3) > 0);
    CHECK(memcmp("abc", "abd", 2) == 0);
    CHECK(memcmp("\x80", "\x01", 1) > 0);       /* bytes compare unsigned */
}

void test_string_memmove_overlap(void)
{
    char up[] = "abcdef";
    memmove(up + 2, up, 4);                     /* dst above src */
    CHECK_STR(up, "ababcd");

    char down[] = "abcdef";
    memmove(down, down + 2, 4);                 /* dst below src */
    CHECK_STR(down, "cdefef");
}

void test_string_str(void)
{
    CHECK_INT(strlen(""), 0);
    CHECK_INT(strlen("sprout"), 6);
    CHECK(strcmp("abc", "abc") == 0);
    CHECK(strcmp("abc", "abd") < 0);
    CHECK(strcmp("ab", "abc") < 0);
    CHECK(strcmp("abc", "ab") > 0);
    CHECK(strncmp("help me", "help", 4) == 0);
    CHECK(strncmp("help", "hello", 5) > 0);
    CHECK(strncmp("anything", "else", 0) == 0);
}
