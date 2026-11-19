/* counter.c — a global variable: how does the code find its address? */
int ticks;

int tick(void)
{
    return ++ticks;
}
