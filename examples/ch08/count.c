
/* count (examples/ch08/count.c): add 1 to a shared counter, ten million
 * times, without disabling interrupts. plus_one() is a real call, like the
 * work real code does between reading shared data and writing it back. */
static volatile uint32_t counter;

__attribute__((noinline)) static uint32_t plus_one(uint32_t v)
{
    return v + 1;
}

static int count(void)
{
    for (uint32_t i = 0; i < 10000000; i++)
        counter = plus_one(counter);    /* load, call, store */
    kprintf("counter = %u\n", (unsigned)counter);
    return 0;
}
