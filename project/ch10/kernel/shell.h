/* shell.h — sprout's command line. */
#ifndef SPROUT_SHELL_H
#define SPROUT_SHELL_H

#define SHELL_PROMPT "sprout> "

/* Read commands and run them, forever: the entry point of task 1. */
__attribute__((noreturn)) int shell_main(void);

#endif
