/* programs.h — the programs the shell can start: kernel threads (Chapter
 * 8) and user programs (Chapter 9). */
#ifndef SPROUT_PROGRAMS_H
#define SPROUT_PROGRAMS_H

#include <stdint.h>

struct program {
    const char *name;
    int (*entry)(void);         /* a kernel thread, or NULL */
    const uint8_t *elf;         /* a user program: its ELF file */
    uint32_t size;
};

/* The program called name, or NULL. */
const struct program *program_find(const char *name);

/* Start p as a new task. Returns its id, or -1 after printing why not. */
int program_start(const struct program *p);

/* Print the names, separated by spaces. */
void program_list(void);

#endif
