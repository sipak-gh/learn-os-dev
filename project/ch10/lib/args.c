/* args.c — see args.h. */
#include "args.h"

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

int args_split(char *line, char *argv[], int max)
{
    int argc = 0;
    char *p = line;

    for (;;) {
        while (is_space(*p))
            *p++ = '\0';
        if (*p == '\0')
            return argc;
        if (argc == max)
            return -1;
        argv[argc++] = p;
        while (*p != '\0' && !is_space(*p))
            p++;
    }
}

static int digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 99;                      /* not a digit in any base we use */
}

bool args_parse_u32(const char *s, uint32_t *out)
{
    uint32_t base = 10, value = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }
    if (*s == '\0')
        return false;

    for (; *s != '\0'; s++) {
        uint32_t d = (uint32_t)digit_value(*s);
        if (d >= base)
            return false;
        if (value > (UINT32_MAX - d) / base)
            return false;           /* value * base + d would overflow */
        value = value * base + d;
    }
    *out = value;
    return true;
}
