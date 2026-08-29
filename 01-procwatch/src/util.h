#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_PATH 256
#define MAX_CMDLINE 4096
#define MAX_COMM 256

char *trim(char *s);
int read_file(const char *path, char *buf, size_t buflen);
int read_file_null_sep(const char *path, char *buf, size_t buflen);
long get_clk_tck(void);
double timeval_diff_sec(const struct timespec *a, const struct timespec *b);
bool is_pid_dir(const char *name);
void die(const char *fmt, ...);
void warn(const char *fmt, ...);

#endif