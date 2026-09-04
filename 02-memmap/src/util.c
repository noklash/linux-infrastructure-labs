#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

bool parse_pid(const char *str, unsigned long *out_pid)
{
    char *end = NULL;
    unsigned long val;

    if (!str || !*str || !out_pid)
        return false;
    if (!isdigit((unsigned char)*str))
        return false;

    errno = 0;
    val = strtoul(str, &end, 10);
    if (errno == ERANGE || end == str || *end != '\0' || val == 0)
        return false;

    *out_pid = val;
    return true;
}

void print_usage(const char *progname)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] PID\n\n"
        "Inspect virtual memory of a running process via /proc.\n\n"
        "Options:\n"
        "  --summary          Memory summary (VSS/RSS/PSS/...)\n"
        "  --libraries        Shared libraries\n"
        "  --heap             Heap details\n"
        "  --stack            Stack details\n"
        "  --exec             Executable mappings only\n"
        "  --write            Writable mappings only\n"
        "  --shared           Shared mappings only\n"
        "  --private          Private mappings only\n"
        "  --anonymous        Anonymous mappings only\n"
        "  --sort KEY         size | rss | pss | start\n"
        "  --json             JSON output\n"
        "  --watch            Live refresh\n"
        "  --interval SEC     Watch interval (default 1)\n"
        "  -h, --help         Show help\n",
        progname ? progname : "memmap");
}

void format_size(unsigned long bytes, char *buf, size_t buflen)
{
    if (bytes < 1024UL)
        snprintf(buf, buflen, "%lu B", bytes);
    else if (bytes < 1024UL * 1024)
        snprintf(buf, buflen, "%.1f KB", bytes / 1024.0);
    else if (bytes < 1024UL * 1024 * 1024)
        snprintf(buf, buflen, "%.2f MB", bytes / (1024.0 * 1024.0));
    else
        snprintf(buf, buflen, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

char *xstrdup(const char *s)
{
    size_t n;
    char *p;

    if (!s)
        return NULL;
    n = strlen(s) + 1;
    p = malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

long get_page_size(void)
{
    long ps = sysconf(_SC_PAGESIZE);
    return (ps > 0) ? ps : 4096;
}