/* canary.c — a local array: a hosted gcc build guards it with a canary. */
#include <stddef.h>

__attribute__((noinline)) void read_name(char *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = i < 6 ? "sprout"[i] : '\0';
}

size_t name_length(void)
{
    char name[32];
    read_name(name, sizeof name);
    size_t n = 0;
    while (n < sizeof name && name[n])
        n++;
    return n;
}
