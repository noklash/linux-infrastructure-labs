#define _POSIX_C_SOURCE 200809L
#include "smaps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static unsigned long parse_kb_field(const char *line, const char *key)
{
    const char *p;
    unsigned long v = 0;

    if (strncmp(line, key, strlen(key)) != 0)
        return 0;
    p = line + strlen(key);
    while (*p == ' ' || *p == '\t')
        p++;
    sscanf(p, "%lu", &v);
    return v * 1024UL;
}

int collect_smaps(unsigned long pid, struct memory_map *mm)
{
    char path[64];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    size_t idx = 0;
    struct memory_mapping *cur = NULL;

    snprintf(path, sizeof(path), "/proc/%lu/smaps", pid);
    fp = fopen(path, "r");
    if (!fp)
        return (errno == EACCES || errno == ENOENT) ? 0 : -1;

    while ((nread = getline(&line, &len, fp)) != -1) {
        unsigned long start, end;

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            cur = NULL;
            while (idx < mm->count) {
                if (mm->items[idx].start == start &&
                    mm->items[idx].end == end) {
                    cur = &mm->items[idx];
                    idx++;
                    break;
                }
                idx++;
            }
            continue;
        }

        if (!cur)
            continue;

        cur->stats.has_smaps = true;
        if (!strncmp(line, "Size:", 5))
            cur->stats.size = parse_kb_field(line, "Size:");
        else if (!strncmp(line, "Rss:", 4))
            cur->stats.rss = parse_kb_field(line, "Rss:");
        else if (!strncmp(line, "Pss:", 4))
            cur->stats.pss = parse_kb_field(line, "Pss:");
        else if (!strncmp(line, "Shared_Clean:", 13))
            cur->stats.shared_clean = parse_kb_field(line, "Shared_Clean:");
        else if (!strncmp(line, "Shared_Dirty:", 13))
            cur->stats.shared_dirty = parse_kb_field(line, "Shared_Dirty:");
        else if (!strncmp(line, "Private_Clean:", 14))
            cur->stats.private_clean = parse_kb_field(line, "Private_Clean:");
        else if (!strncmp(line, "Private_Dirty:", 14))
            cur->stats.private_dirty = parse_kb_field(line, "Private_Dirty:");
        else if (!strncmp(line, "Referenced:", 11))
            cur->stats.referenced = parse_kb_field(line, "Referenced:");
        else if (!strncmp(line, "Anonymous:", 10))
            cur->stats.anonymous = parse_kb_field(line, "Anonymous:");
        else if (!strncmp(line, "Swap:", 5))
            cur->stats.swap = parse_kb_field(line, "Swap:");
    }

    free(line);
    fclose(fp);
    return 0;
}