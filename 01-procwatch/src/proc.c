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

    if (!t->procs)
        return -1;

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
    if (t->count < t->capacity)
        return 0;

    int ncap = t->capacity * 2;

    if (ncap > MAX_PROCS)
        ncap = MAX_PROCS;

    if (t->count >= ncap)
        return -1;

    struct proc_info *np =
        realloc(t->procs, ncap * sizeof(struct proc_info));

    if (!np)
        return -1;

    t->procs = np;
    t->capacity = ncap;

    return 0;
}

/*
 * Parse /proc/<pid>/stat.
 *
 * The important detail is that field 2, comm, is enclosed in
 * parentheses and may contain spaces. Therefore we cannot simply
 * split the entire line on whitespace.
 *
 * Fields used:
 *
 *   1  pid
 *   2  comm
 *   3  state
 *   4  ppid
 *   5  pgrp
 *   6  session
 *   7  tty_nr
 *   8  tpgid
 *   9  flags
 *  10  minflt
 *  11  cminflt
 *  12  majflt
 *  13  cmajflt
 *  14  utime
 *  15  stime
 *  16  cutime
 *  17  cstime
 *  18  priority
 *  19  nice
 *  20  num_threads
 *  21  itrealvalue
 *  22  starttime
 *  23  vsize
 *  24  rss
 */
static int parse_stat(const char *buf, struct proc_info *p)
{
    /*
     * Find the last ')' that closes the comm field.
     *
     * We use the last ')' rather than the first ')' because the
     * process name can theoretically contain ')' characters.
     */
    const char *rparen = strrchr(buf, ')');

    if (!rparen)
        return -1;

    /*
     * PID is the first field, before the opening '('.
     */
    if (sscanf(buf, "%d", &p->pid) != 1)
        return -1;

    /*
     * Find the opening '(' of the comm field.
     */
    const char *lparen = strchr(buf, '(');

    if (!lparen || lparen >= rparen)
        return -1;

    /*
     * Extract comm between '(' and ')'.
     */
    size_t len = (size_t)(rparen - lparen - 1);

    if (len >= MAX_COMM)
        len = MAX_COMM - 1;

    memcpy(p->name, lparen + 1, len);
    p->name[len] = '\0';

    /*
     * Everything after ") " begins with field 3:
     *
     * field 3  = state
     * field 4  = ppid
     * field 5  = pgrp
     * ...
     */
    const char *rest = rparen + 2;

    char state;
    int ppid;

    unsigned long utime;
    unsigned long stime;

    int num_threads;

    unsigned long long starttime;

    unsigned long vsize;

    long rss;

    /*
     * Parse the fields we actually need.
     *
     * After state and ppid:
     *
     * fields 5-9:
     *   pgrp
     *   session
     *   tty_nr
     *   tpgid
     *   flags
     *
     * fields 10-13:
     *   minflt
     *   cminflt
     *   majflt
     *   cmajflt
     *
     * field 14:
     *   utime
     *
     * field 15:
     *   stime
     *
     * fields 16-19:
     *   cutime
     *   cstime
     *   priority
     *   nice
     *
     * field 20:
     *   num_threads
     *
     * field 21:
     *   itrealvalue
     *
     * field 22:
     *   starttime
     *
     * field 23:
     *   vsize
     *
     * field 24:
     *   rss
     */
    int n = sscanf(
        rest,
        "%c %d "
        "%*d %*d %*d %*d %*u "
        "%*lu %*lu %*lu %*lu "
        "%lu %lu "
        "%*ld %*ld %*ld %*ld "
        "%d %*d "
        "%llu %lu %ld",
        &state,
        &ppid,
        &utime,
        &stime,
        &num_threads,
        &starttime,
        &vsize,
        &rss
    );

    if (n != 8)
        return -1;

    p->state = state;
    p->ppid = ppid;

    p->utime = utime;
    p->stime = stime;

    p->threads = num_threads;

    p->starttime = starttime;

    p->vsize = vsize;
    p->rss = rss;

    return 0;
}

int proc_read_one(int pid, struct proc_info *out)
{
    char path[MAX_PATH];
    char buf[4096];

    memset(out, 0, sizeof(*out));

    out->pid = pid;
    out->valid = false;

    /*
     * Read /proc/<pid>/stat.
     */
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    if (read_file(path, buf, sizeof(buf)) < 0)
        return -1;

    if (parse_stat(buf, out) < 0)
        return -1;

    /*
     * Read /proc/<pid>/cmdline.
     *
     * Arguments in cmdline are separated by NUL characters rather
     * than spaces, so read_file_null_sep() converts them for display.
     */
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    if (read_file_null_sep(
            path,
            out->cmdline,
            sizeof(out->cmdline)) < 0) {

        /*
         * Kernel threads generally have an empty cmdline.
         * Fall back to the process name.
         */
        strncpy(
            out->cmdline,
            out->name,
            sizeof(out->cmdline) - 1
        );

        out->cmdline[sizeof(out->cmdline) - 1] = '\0';
    }

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

        /*
         * /proc contains many non-process entries:
         *
         *   /proc/sys
         *   /proc/meminfo
         *   /proc/cpuinfo
         *
         * We only want directories whose names consist entirely
         * of digits.
         */
        if (!is_pid_dir(ent->d_name))
            continue;

        int pid = atoi(ent->d_name);

        if (pid <= 0)
            continue;

        if (ensure_capacity(t) < 0)
            break;

        struct proc_info *p = &t->procs[t->count];

        /*
         * A process can disappear between readdir() and opening
         * /proc/<pid>/stat.
         *
         * That is normal on Linux, so silently skip it.
         */
        if (proc_read_one(pid, p) == 0)
            t->count++;
    }

    closedir(dir);

    return t->count;
}

void proc_calc_cpu(
    struct proc_table *prev,
    struct proc_table *curr,
    double elapsed_sec)
{
    if (elapsed_sec <= 0.0)
        elapsed_sec = 1.0;

    long tck = get_clk_tck();

    for (int i = 0; i < curr->count; i++) {

        struct proc_info *c = &curr->procs[i];

        c->cpu_pct = 0.0;

        /*
         * Find the same PID in the previous sample.
         */
        struct proc_info *p = proc_find(prev, c->pid);

        if (!p || !p->valid)
            continue;

        /*
         * CPU time is measured in clock ticks.
         *
         * utime = user CPU time
         * stime = system CPU time
         */
        unsigned long long curr_ticks =
            (unsigned long long)c->utime +
            (unsigned long long)c->stime;

        unsigned long long prev_ticks =
            (unsigned long long)p->utime +
            (unsigned long long)p->stime;

        /*
         * If the process was restarted with the same PID,
         * its counters may have reset.
         */
        if (curr_ticks < prev_ticks)
            continue;

        unsigned long long delta =
            curr_ticks - prev_ticks;

        /*
         * Convert clock ticks to seconds and calculate the
         * percentage of one CPU used during the sample.
         */
        c->cpu_pct =
            100.0 *
            ((double)delta / tck) /
            elapsed_sec;
    }
}

static int cmp_cpu(const void *a, const void *b)
{
    const struct proc_info *pa = a;
    const struct proc_info *pb = b;

    if (pa->cpu_pct < pb->cpu_pct)
        return 1;

    if (pa->cpu_pct > pb->cpu_pct)
        return -1;

    return 0;
}

static int cmp_mem(const void *a, const void *b)
{
    const struct proc_info *pa = a;
    const struct proc_info *pb = b;

    if (pa->rss < pb->rss)
        return 1;

    if (pa->rss > pb->rss)
        return -1;

    return 0;
}

void proc_sort_cpu(struct proc_table *t)
{
    qsort(
        t->procs,
        t->count,
        sizeof(struct proc_info),
        cmp_cpu
    );
}

void proc_sort_mem(struct proc_table *t)
{
    qsort(
        t->procs,
        t->count,
        sizeof(struct proc_info),
        cmp_mem
    );
}

struct proc_info *proc_find(
    struct proc_table *t,
    int pid)
{
    for (int i = 0; i < t->count; i++) {

        if (t->procs[i].pid == pid)
            return &t->procs[i];
    }

    return NULL;
}