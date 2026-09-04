#ifndef MEMMAP_UTIL_H
#define MEMMAP_UTIL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

bool parse_pid(const char *str, unsigned long *out_pid);
void print_usage(const char *progname);

/* Human-readable size (e.g. "1.5 MB"). buf must be >= 32 bytes. */
void format_size(unsigned long bytes, char *buf, size_t buflen);

/* Safe strdup; returns NULL on failure. */
char *xstrdup(const char *s);

long get_page_size(void);

#endif