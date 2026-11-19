/* test_memory.c - tests for lib/bitmap.c and lib/heap.c. */
#include <stdint.h>

#include "test.h"
#include "bitmap.h"
#include "heap.h"

void test_bitmap_bits(void)
{
    uint32_t map[BITMAP_WORDS(70)] = { 0 };

    CHECK_INT(BITMAP_WORDS(70), 3);
    bitmap_set(map, 0);
    bitmap_set(map, 33);
    bitmap_set(map, 69);
    CHECK(bitmap_test(map, 0) && bitmap_test(map, 33) && bitmap_test(map, 69));
    CHECK(!bitmap_test(map, 1) && !bitmap_test(map, 32));
    CHECK_INT(map[1], 2);
    bitmap_clear(map, 33);
    CHECK_INT(map[1], 0);
}

void test_bitmap_find_clear(void)
{
    uint32_t map[BITMAP_WORDS(70)];

    map[0] = 0xFFFFFFFFu;               /* bits 0-31 used */
    map[1] = 0x0000FFFFu;               /* 32-47 used */
    map[2] = 0;
    CHECK_INT(bitmap_find_clear(map, 70, 0), 48);
    CHECK_INT(bitmap_find_clear(map, 70, 50), 50);
    map[1] = 0xFFFFFFFFu;
    map[2] = 0x3Fu;                     /* 64-69 used: all 70 bits */
    CHECK_INT(bitmap_find_clear(map, 70, 0), 70);
}

/* A heap in a static array, handed out by a fake sbrk. */
static uint8_t arena[64 * 1024] __attribute__((aligned(8)));
static size_t arena_used, arena_limit;

static void *arena_more(size_t n)
{
    if (arena_used + n > arena_limit)
        return NULL;
    void *p = arena + arena_used;
    arena_used += n;
    return p;
}

static void heap_setup(struct heap *h, size_t limit)
{
    arena_used = 0;
    arena_limit = limit;
    CHECK(heap_init(h, arena_more));
}

static void count_block(void *payload, size_t size, bool used, void *arg)
{
    (void)payload, (void)size, (void)used;
    (*(int *)arg)++;
}

static int blocks(const struct heap *h)
{
    int n = 0;
    heap_walk(h, count_block, &n);
    return n;
}

void test_heap_alloc(void)
{
    struct heap h;
    heap_setup(&h, sizeof arena);

    CHECK(heap_alloc(&h, 0) == NULL);
    uint8_t *a = heap_alloc(&h, 100);
    uint8_t *b = heap_alloc(&h, 1);
    CHECK(a == arena + 16);             /* after padding, prologue, header */
    CHECK_INT(b - a, 112);              /* 100 + 8 rounded up to 8 */
    CHECK_INT((uintptr_t)b % 8, 0);
    CHECK_INT(arena_used, 16 + HEAP_CHUNK);
    CHECK_INT(blocks(&h), 3);           /* a, b, the rest of the chunk */
    for (int i = 0; i < 100; i++)
        a[i] = 0xAA;                    /* must not touch b */
    b[0] = 1;
    CHECK(heap_check(&h));
}

void test_heap_free_coalesces(void)
{
    struct heap h;
    heap_setup(&h, sizeof arena);

    void *a = heap_alloc(&h, 32);
    void *b = heap_alloc(&h, 32);
    void *c = heap_alloc(&h, 32);
    CHECK_INT(blocks(&h), 4);
    CHECK(heap_free(&h, a));
    CHECK(heap_free(&h, c));            /* merges with the free tail */
    CHECK_INT(blocks(&h), 3);           /* a, b, c + tail */
    CHECK(heap_free(&h, b));            /* merges on both sides */
    CHECK_INT(blocks(&h), 1);
    CHECK(heap_check(&h));
    CHECK(heap_alloc(&h, 64) == a);     /* first fit: from the start again */
}

void test_heap_bad_free(void)
{
    struct heap h;
    heap_setup(&h, sizeof arena);

    uint8_t *a = heap_alloc(&h, 32);
    CHECK(!heap_free(&h, NULL));
    CHECK(!heap_free(&h, a + 8));       /* not the start of a block */
    CHECK(heap_free(&h, a));
    CHECK(!heap_free(&h, a));           /* double free */
    CHECK(heap_check(&h));
}

void test_heap_grows(void)
{
    struct heap h;
    heap_setup(&h, 3 * HEAP_CHUNK);

    void *a = heap_alloc(&h, 3000);
    void *b = heap_alloc(&h, 3000);     /* doesn't fit the rest: grow */
    CHECK(a != NULL && b != NULL);
    CHECK_INT(arena_used, 16 + 2 * HEAP_CHUNK);
    CHECK(heap_alloc(&h, 3 * HEAP_CHUNK) == NULL);  /* more() says no */
    CHECK(heap_free(&h, a));
    CHECK(heap_free(&h, b));
    CHECK_INT(blocks(&h), 1);           /* one free block, 2 chunks */
    CHECK(heap_alloc(&h, 2 * HEAP_CHUNK - 8) != NULL);
    CHECK(heap_check(&h));
}
