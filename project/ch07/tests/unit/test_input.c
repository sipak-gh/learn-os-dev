/* test_input.c - tests for lib/ringbuf.c, lib/line.c and lib/args.c. */
#include <stdint.h>

#include "test.h"
#include "args.h"
#include "line.h"
#include "ringbuf.h"

void test_ringbuf_fifo(void)
{
    struct ringbuf rb;
    char c = 0;

    ringbuf_init(&rb);
    CHECK(!ringbuf_get(&rb, &c));
    CHECK(ringbuf_put(&rb, 'a'));
    CHECK(ringbuf_put(&rb, 'b'));
    CHECK_INT(ringbuf_count(&rb), 2);
    CHECK(ringbuf_get(&rb, &c) && c == 'a');
    CHECK(ringbuf_get(&rb, &c) && c == 'b');
    CHECK(!ringbuf_get(&rb, &c));
}

void test_ringbuf_full(void)
{
    struct ringbuf rb;
    char c = 0;

    ringbuf_init(&rb);
    for (int i = 0; i < RINGBUF_SIZE; i++)
        CHECK(ringbuf_put(&rb, (char)i));
    CHECK(!ringbuf_put(&rb, 'x'));              /* full: dropped */
    CHECK_INT(ringbuf_count(&rb), RINGBUF_SIZE);
    CHECK(ringbuf_get(&rb, &c) && c == 0);
    CHECK(ringbuf_put(&rb, 'y'));               /* room again */
}

void test_ringbuf_counter_wrap(void)
{
    struct ringbuf rb;
    char c = 0;

    rb.head = rb.tail = UINT32_MAX - 1;         /* about to wrap */
    for (int i = 0; i < 4; i++)
        CHECK(ringbuf_put(&rb, (char)('0' + i)));
    CHECK_INT(ringbuf_count(&rb), 4);
    for (int i = 0; i < 4; i++)
        CHECK(ringbuf_get(&rb, &c) && c == '0' + i);
    CHECK_INT(ringbuf_count(&rb), 0);
}

static char echoed[64];
static size_t n_echoed;

static void echo(char c)
{
    if (n_echoed + 1 < sizeof echoed)
        echoed[n_echoed++] = c;
    echoed[n_echoed] = '\0';
}

/* Feed every character of s; return how many lines it completed. */
static int feed(struct line *l, const char *s)
{
    int lines = 0;
    for (; *s; s++)
        lines += line_feed(l, *s);
    return lines;
}

void test_line_editing(void)
{
    char buf[16];
    struct line l;

    n_echoed = 0;
    line_init(&l, buf, sizeof buf, echo);
    CHECK_INT(feed(&l, "helo\bp"), 0);
    CHECK_STR(echoed, "helo\b \bp");
    CHECK_INT(feed(&l, "\n"), 1);
    CHECK_STR(buf, "help");

    n_echoed = 0;
    CHECK_INT(feed(&l, "\b\x7f" "ab\x7f\x7f\x7f" "c\n"), 1); /* too many erases */
    CHECK_STR(buf, "c");
    CHECK_STR(echoed, "ab\b \b\b \bc\n");
}

void test_line_limits(void)
{
    char buf[4];
    struct line l;

    n_echoed = 0;
    line_init(&l, buf, sizeof buf, echo);
    CHECK_INT(feed(&l, "abcdef\n"), 1);         /* only 3 fit */
    CHECK_STR(buf, "abc");
    CHECK_INT(feed(&l, "\x01\t\x1b" "x\n"), 1); /* control characters ignored */
    CHECK_STR(buf, "x");
    CHECK_INT(feed(&l, "\n"), 1);               /* an empty line is a line */
    CHECK_STR(buf, "");
}

void test_args_split(void)
{
    char line[] = "  echo  hello\tworld ";
    char *argv[4];

    CHECK_INT(args_split(line, argv, 4), 3);
    CHECK_STR(argv[0], "echo");
    CHECK_STR(argv[1], "hello");
    CHECK_STR(argv[2], "world");

    char empty[] = "   ";
    CHECK_INT(args_split(empty, argv, 4), 0);

    char many[] = "a b c";
    CHECK_INT(args_split(many, argv, 2), -1);
}

void test_args_parse_u32(void)
{
    uint32_t v = 7;

    CHECK(args_parse_u32("0", &v) && v == 0);
    CHECK(args_parse_u32("250", &v) && v == 250);
    CHECK(args_parse_u32("0x1F", &v) && v == 31);
    CHECK(args_parse_u32("4294967295", &v) && v == UINT32_MAX);
    v = 7;
    CHECK(!args_parse_u32("4294967296", &v));   /* overflow */
    CHECK(!args_parse_u32("", &v));
    CHECK(!args_parse_u32("0x", &v));
    CHECK(!args_parse_u32("12a", &v));
    CHECK(!args_parse_u32("-1", &v));
    CHECK_INT(v, 7);
}
