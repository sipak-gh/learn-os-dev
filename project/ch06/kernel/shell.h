/* shell.h — sprout's command line. */
#ifndef SPROUT_SHELL_H
#define SPROUT_SHELL_H

#include "boot_info.h"

#define SHELL_PROMPT "sprout> "

/* Read commands and run them, forever. */
__attribute__((noreturn)) void shell_run(const struct boot_info *bi);

#endif
