#include "proc.h"
#include "maps.h"
#include "smaps.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int proc_read_comm(unsigned long pid, char **out_comm)
{
    char path[64];
    FILE *fp;
    char buf[256];

    snprintf(path, sizeof(path), "/proc/%lu/comm", pid);
    fp = fopen(path, "r");
    if (!fp)
        return -1;
    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    buf[strcspn(buf, "\n")] = '\0';
    *out_comm = xstrdup(buf);
    return *out_comm ? 0 : -1;
}

int collect_process_memory(unsigned long pid, struct memory_map *mm)
{
    char path[64];
    struct stat st;

    snprintf(path, sizeof(path), "/proc/%lu", pid);
    if (stat(path, &st) != 0) {
        if (errno == ENOENT)
            fprintf(stderr, "Error: process %lu does not exist.\n", pid);
        else if (errno == EACCES)
            fprintf(stderr, "Permission denied while inspecting PID %lu.\n", pid);
        else
            fprintf(stderr, "Error: cannot access /proc/%lu: %s\n",
                    pid, strerror(errno));
        return -1;
    }

    mm->pid = pid;
    proc_read_comm(pid, &mm->comm);

    if (collect_maps(pid, mm) != 0)
        return -1;

    collect_smaps(pid, mm); /* best-effort */
    return 0;
}