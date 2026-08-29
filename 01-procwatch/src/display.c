#define _POSIX_C_SOURCE 200809L
#include "display.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

static long page_size(void)
{
    static long ps = 0;
    if (ps == 0) {
        ps = sysconf(_SC_PAGESIZE);
        if (ps <= 0) ps = 4096;
    }
    return ps;
}

void display_table(struct proc_table *t, int limit)
{
    printf("%-7s %-7s %-5s %-6s %8s %8s %6s %s\n",
           "PID", "PPID", "STATE", "THR", "VSIZE", "RSS", "CPU%", "COMMAND");
    printf("------- ------- ----- ------ -------- -------- ------ ----------------\n");

    int shown = 0;
    for (int i = 0; i < t->count && (limit <= 0 || shown < limit); i++) {
        struct proc_info *p = &t->procs[i];
        if (!p->valid) continue;

        unsigned long rss_kb = (p->rss * page_size()) / 1024;
        unsigned long vsz_kb = p->vsize / 1024;

        const char *cmd = p->cmdline[0] ? p->cmdline : p->name;
        /* truncate for display */
        char shortcmd[64];
        strncpy(shortcmd, cmd, sizeof(shortcmd) - 1);
        shortcmd[sizeof(shortcmd) - 1] = '\0';

        printf("%-7d %-7d %-5c %-6d %7luk %7luk %5.1f%% %s\n",
               p->pid, p->ppid, p->state, p->threads,
               vsz_kb, rss_kb, p->cpu_pct, shortcmd);
        shown++;
    }
}

void display_one(const struct proc_info *p)
{
    unsigned long rss_kb = (p->rss * page_size()) / 1024;
    unsigned long vsz_kb = p->vsize / 1024;

    printf("PID:        %d\n", p->pid);
    printf("PPID:       %d\n", p->ppid);
    printf("Name:       %s\n", p->name);
    printf("State:      %c\n", p->state);
    printf("Threads:    %d\n", p->threads);
    printf("VSIZE:      %lu kB\n", vsz_kb);
    printf("RSS:        %lu kB\n", rss_kb);
    printf("CPU time:   %lu jiffies (user) + %lu jiffies (sys)\n",
           p->utime, p->stime);
    printf("CPU%%:       %.1f\n", p->cpu_pct);
    printf("Cmdline:    %s\n", p->cmdline[0] ? p->cmdline : "(none)");
}

/* Simple tree: print children recursively */
static void print_tree_recursive(struct proc_table *t, int ppid, int depth, bool *printed)
{
    for (int i = 0; i < t->count; i++) {
        struct proc_info *p = &t->procs[i];
        if (!p->valid || p->ppid != ppid) continue;
        if (printed[i]) continue; /* avoid cycles */
        printed[i] = true;

        for (int d = 0; d < depth; d++) printf("  ");
        printf("%d %s (%c)\n", p->pid, p->name, p->state);
        print_tree_recursive(t, p->pid, depth + 1, printed);
    }
}

void display_tree(struct proc_table *t)
{
    bool *printed = calloc(t->count, sizeof(bool));
    if (!printed) return;

    /* Roots are processes whose parent is not in the table (or pid 0/1) */
    for (int i = 0; i < t->count; i++) {
        struct proc_info *p = &t->procs[i];
        if (!p->valid) continue;
        if (proc_find(t, p->ppid) == NULL) {
            if (!printed[i]) {
                printed[i] = true;
                printf("%d %s (%c)\n", p->pid, p->name, p->state);
                print_tree_recursive(t, p->pid, 1, printed);
            }
        }
    }
    free(printed);
}

void display_fds(int pid)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/fd", pid);
    DIR *dir = opendir(path);
    if (!dir) {
        warn("cannot open %s: %s", path, strerror(errno));
        return;
    }

    printf("FD   TARGET\n");
    printf("---  --------------------------------\n");
    struct dirent *ent;
    char linkbuf[MAX_PATH];
    char target[MAX_PATH];
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        snprintf(linkbuf, sizeof(linkbuf), "/proc/%d/fd/%s", pid, ent->d_name);
        ssize_t n = readlink(linkbuf, target, sizeof(target) - 1);
        if (n < 0) {
            printf("%-4s (error: %s)\n", ent->d_name, strerror(errno));
            continue;
        }
        target[n] = '\0';
        printf("%-4s %s\n", ent->d_name, target);
    }
    closedir(dir);
}

void display_maps(int pid)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        warn("cannot open %s: %s", path, strerror(errno));
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        fputs(line, stdout);
    }
    fclose(f);
}