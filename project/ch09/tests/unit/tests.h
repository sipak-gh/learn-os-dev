/* tests.h - the list of every unit test, as an X-macro.
 *
 * To add a test: write void test_NAME(void) in a test_*.c file and add
 * X(NAME) here. unit.c expands this list into a table, and the line below
 * declares every test function, so a typo is a compile error.
 */
#ifndef SPROUT_TESTS_H
#define SPROUT_TESTS_H

#define TESTS(X)                    \
    X(string_mem)                   \
    X(string_memmove_overlap)       \
    X(string_str)                   \
    X(fmt_divmod64)                 \
    X(fmt_integers)                 \
    X(fmt_width_and_flags)          \
    X(fmt_64bit)                    \
    X(fmt_strings_and_chars)        \
    X(fmt_snprintf_truncates)       \
    X(ringbuf_fifo)                 \
    X(ringbuf_full)                 \
    X(ringbuf_counter_wrap)         \
    X(line_editing)                 \
    X(line_limits)                  \
    X(args_split)                   \
    X(args_parse_u32)               \
    X(bitmap_bits)                  \
    X(bitmap_find_clear)            \
    X(heap_alloc)                   \
    X(heap_free_coalesces)          \
    X(heap_bad_free)                \
    X(heap_grows)

#define TEST_DECLARE(name) void test_##name(void);
TESTS(TEST_DECLARE)
#undef TEST_DECLARE

#endif
