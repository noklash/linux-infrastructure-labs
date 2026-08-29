#define _POSIX_C_SOURCE 200809L
#include "proc.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

int proc_table_init(struct proc_table *t)
{
    t->capacity = 512;
    t->count = 0;
    t->procs = calloc(t->capacity, sizeof(struct proc_info));
    if (!t->procs) return -1;
    return 0;
}

void proc_table_free(struct proc_table *t)
{
    free(t->procs);
    t->procs = NULL;
    t->count = 0;
    t->capacity = 0;
}

static int ensure_capacity(struct proc_table *t)
{
    if (t->count < t->capacity) return 0;
    int ncap = t->capacity * 2;
    if (ncap > MAX_PROCS) ncap = MAX_PROCS;
    if (t->count >= ncap) return -1;
    struct proc_info *np = realloc(t->procs, ncap * sizeof(struct proc_info));
    if (!np) return -1;
    t->procs = np;
    t->capacity = ncap;
    return 0;
}

/* Parse /proc/pid/stat – fields after comm are space-separated.
 * comm itself is in parentheses and may contain spaces. */
static int parse_stat(const char *buf, struct proc_info *p)
{
    /* Find the last ')' that closes the comm field */
    const char *rparen = strrchr(buf, ')');
    if (!rparen) return -1;

    /* PID is before the first '(' */
    if (sscanf(buf, "%d", &p->pid) != 1) return -1;

    /* Extract comm between ( and ) */
    const char *lparen = strchr(buf, '(');
    if (!lparen || lparen >= rparen) return -1;
    size_t len = (size_t)(rparen - lparen - 1);
    if (len >= MAX_COMM) len = MAX_COMM - 1;
    memcpy(p->name, lparen + 1, len);
    p->name[len] = '\0';

    /* After ') ' the remaining fields start */
    const char *rest = rparen + 2; /* skip ") " */
    char state;
    int ppid;
    unsigned long utime, stime, vsize, rss;
    unsigned long long starttime;
    int num_threads;
    /* Field numbers (1-based after pid): state=3, ppid=4, ... utime=14, stime=15,
       ... num_threads=20, starttime=22, vsize=23, rss=24 */
    int n = sscanf(rest,
                   "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
                   "%lu %lu %*d %*d %*d %*d %*d %*d %d %*d %llu %lu %lu",
                   &state, &ppid,
                   &utime, &stime,
                   &num_threads,
                   &starttime, &vsize, &rss);
    if (n < 8) return -1;

    p->state = state;
    p->ppid = ppid;
    p->utime = utime;
    p->stime = stime;
    p->threads = num_threads;
    p->starttime = starttime;
    p->vsize = vsize;
    p->rss = rss; /* pages */
    return 0;
}

int proc_read_one(int pid, struct proc_info *out)
{
    char path[MAX_PATH];
    char buf[4096];
    memset(out, 0, sizeof(*out));
    out->pid = pid;
    out->valid = false;

    /* stat */
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    if (read_file(path, buf, sizeof(buf)) < 0) return -1;
    if (parse_stat(buf, out) < 0) return -1;

    /* cmdline (best effort) */
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    if (read_file_null_sep(path, out->cmdline, sizeof(out->cmdline)) < 0) {
        /* kernel threads have empty cmdline; fall back to name */
        strncpy(out->cmdline, out->name, sizeof(out->cmdline) - 1);
    }

    /* Optionally cross-check status for VmRSS etc. – not required for core */
    out->valid = true;
    return 0;
}

int proc_scan(struct proc_table *t)
{
    DIR *dir = opendir("/proc");
    if (!dir) {
        warn("cannot open /proc: %s", strerror(errno));
        return -1;
    }

    t->count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!is_pid_dir(ent->d_name)) continue;
        int pid = atoi(ent->d_name);
        if (pid <= 0) continue;

        if (ensure_capacity(t) < 0) break;

        struct proc_info *p = &t->procs[t->count];
        if (proc_read_one(pid, p) == 0) {
            t->count++;
        }
        /* silently skip processes that disappeared */
    }
    closedir(dir);
    return t->count;
}

void proc_calc_cpu(struct proc_table *prev, struct proc_table *curr, double elapsed_sec)
{
    if (elapsed_sec <= 0.0) elapsed_sec = 1.0;
    long tck = get_clk_tck();

    for (int i = 0; i < curr->count; i++) {
        struct proc_info *c = &curr->procs[i];
        c->cpu_pct = 0.0;
        struct proc_info *p = proc_find(prev, c->pid);
        if (!p || !p->valid) continue;

        unsigned long long delta = (c->utime + c->stime) - (p->utime + p->stime);
        /* percentage of one CPU */
        c->cpu_pct = 100.0 * ((double)delta / tck) / elapsed_sec;
    }
}

static int cmp_cpu(const void *a, const void *b)
{
    const struct proc_info *pa = a, *pb = b;
    if (pa->cpu_pct < pb->cpu_pct) return 1;
    if (pa->cpu_pct > pb->cpu_pct) return -1;
    return 0;
}

static int cmp_mem(const void *a, const void *b)
{
    const struct proc_info *pa = a, *pb = b;
    if (pa->rss < pb->rss) return 1;
    if (pa->rss > pb->rss) return -1;
    return 0;
}

void proc_sort_cpu(struct proc_table *t)
{
    qsort(t->procs, t->count, sizeof(struct proc_info), cmp_cpu);
}

void proc_sort_mem(struct proc_table *t)
{
    qsort(t->procs, t->count, sizeof(struct proc_info), cmp_mem);
}

struct proc_info *proc_find(struct proc_table *t, int pid)
{
    for (int i = 0; i < t->count; i++) {
        if (t->procs[i].pid == pid) return &t->procs[i];
    }
    return NULL;
}