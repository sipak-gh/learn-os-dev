/* args.h — split a command line into words; parse numbers. */
#ifndef SPROUT_ARGS_H
#define SPROUT_ARGS_H

#include <stdbool.h>
#include <stdint.h>

/* Split line in place at spaces and tabs: argv[i] points at word i, each
 * NUL-terminated. Returns the number of words, or -1 if there are more
 * than max (argv then holds the first max). */
int args_split(char *line, char *argv[], int max);

/* Parse a decimal or 0x-prefixed hexadecimal number that fits in 32 bits.
 * Returns false (and leaves *out alone) for anything else. */
bool args_parse_u32(const char *s, uint32_t *out);

#endif
