/* spin.c — never makes a system call (Chapter 9).
 *
 * The timer still interrupts it, so the shell keeps running, and `kill`
 * stops it.
 */
int main(void)
{
    for (;;)
        ;
}
