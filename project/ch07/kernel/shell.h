/* shell.h — sprout's command line. */
#ifndef SPROUT_SHELL_H
#define SPROUT_SHELL_H

#define SHELL_PROMPT "sprout> "

/* Read commands and run them, forever. */
__attribute__((noreturn)) void shell_run(void);

#endif
