/* programs.h — the kernel threads the shell can start (Chapter 8). */
#ifndef SPROUT_PROGRAMS_H
#define SPROUT_PROGRAMS_H

struct program {
    const char *name;
    int (*entry)(void);
};

/* The program called name, or NULL. */
const struct program *program_find(const char *name);

/* Print the names, separated by spaces. */
void program_list(void);

#endif
