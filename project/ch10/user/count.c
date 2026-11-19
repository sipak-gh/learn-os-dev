/* count.c — two copies of this program, one variable each (Chapter 9).
 *
 * n is at the same address in every copy, but each copy has its own
 * address space, so each has its own n. Run two at once: `run count count`.
 */
#include "ulib.h"

static int n;

int main(void)
{
    int pid = getpid();
    unsigned short cs;

    __asm__ ("mov %%cs, %0" : "=r"(cs));
    printf("count %d: ring %d, n is at %p\n", pid, cs & 3, (void *)&n);
    for (int i = 0; i < 3; i++) {
        n += pid;
        printf("count %d: n = %d\n", pid, n);
        sleep_ms(100);
    }
    return n;
}
