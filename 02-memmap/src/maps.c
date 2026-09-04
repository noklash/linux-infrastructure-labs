#define _POSIX_C_SOURCE 200809L

#include "maps.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int parse_maps_line(const char *line, struct memory_mapping *out)
{
    unsigned long start, end, offset, inode;
    char perms[8];
    char device[32];
    char pathbuf[4096];
    int n;

    memset(out, 0, sizeof(*out));
    pathbuf[0] = '\0';

    n = sscanf(line, "%lx-%lx %4s %lx %31s %lu %4095[^\n]",
               &start, &end, perms, &offset, device, &inode, pathbuf);

    if (n < 6)
        return -1;

    out->start = start;
    out->end = end;
    out->offset = offset;
    out->inode = inode;
    out->size = (end > start) ? (end - start) : 0;

    strncpy(out->perms, perms, 4);
    out->perms[4] = '\0';

    strncpy(out->device, device, sizeof(out->device) - 1);
    out->device[sizeof(out->device) - 1] = '\0';

    if (n >= 7 && pathbuf[0]) {
        char *p = pathbuf;

        while (*p == ' ' || *p == '\t')
            p++;

        if (*p)
            out->pathname = xstrdup(p);
    }

    return 0;
}

int collect_maps(unsigned long pid, struct memory_map *mm)
{
    char path[64];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    snprintf(path, sizeof(path), "/proc/%lu/maps", pid);

    fp = fopen(path, "r");

    if (!fp) {
        if (errno == ENOENT)
            fprintf(stderr,
                    "Process %lu exited during inspection.\n",
                    pid);
        else if (errno == EACCES)
            fprintf(stderr,
                    "Permission denied while inspecting PID %lu.\n",
                    pid);
        else
            fprintf(stderr,
                    "Cannot open %s: %s\n",
                    path,
                    strerror(errno));

        return -1;
    }

    while ((nread = getline(&line, &len, fp)) != -1) {
        struct memory_mapping m;

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        if (parse_maps_line(line, &m) != 0) {
            free(m.pathname);
            continue;
        }

        if (memory_map_push(mm, &m) != 0) {
            free(m.pathname);
            free(line);
            fclose(fp);
            return -1;
        }
    }

    free(line);
    fclose(fp);

    return 0;
}