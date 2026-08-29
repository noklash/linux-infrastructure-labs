#define _POSIX_C_SOURCE 200809L
#include "util.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

char *trim(char *s)
{
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    e[1] = '\0';
    return s;
}

int read_file(const char *path, char *buf, size_t buflen)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, buflen - 1, f);
    buf[n] = '\0';
    fclose(f);
    return (int)n;
}

/* cmdline is null-separated; convert to spaces for display */
int read_file_null_sep(const char *path, char *buf, size_t buflen)
{
    int n = read_file(path, buf, buflen);
    if (n < 0) return -1;
    for (int i = 0; i < n; i++) {
        if (buf[i] == '\0') buf[i] = ' ';
    }
    /* strip trailing spaces */
    while (n > 0 && (buf[n-1] == ' ' || buf[n-1] == '\0')) {
        buf[--n] = '\0';
    }
    return n;
}

long get_clk_tck(void)
{
    static long tck = 0;
    if (tck == 0) {
        tck = sysconf(_SC_CLK_TCK);
        if (tck <= 0) tck = 100; /* fallback */
    }
    return tck;
}

double timeval_diff_sec(const struct timespec *a, const struct timespec *b)
{
    return (b->tv_sec - a->tv_sec) + (b->tv_nsec - a->tv_nsec) / 1e9;
}

bool is_pid_dir(const char *name)
{
    if (!name || !*name) return false;
    for (const char *p = name; *p; p++) {
        if (!isdigit((unsigned char)*p)) return false;
    }
    return true;
}

void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "procwatch: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "procwatch: warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}