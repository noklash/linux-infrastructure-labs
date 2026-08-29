#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include <stdbool.h>
#include "util.h"

#define MAX_PROCS 8192

struct proc_info {
    int     pid;
    int     ppid;
    char    state;
    char    name[MAX_COMM];
    char    cmdline[MAX_CMDLINE];
    int     threads;
    unsigned long vsize;          /* virtual size in bytes */
    unsigned long rss;            /* resident set size in pages */
    unsigned long utime;          /* user jiffies */
    unsigned long stime;          /* system jiffies */
    unsigned long long starttime; /* start time in jiffies */
    double  cpu_pct;              /* filled after second sample */
    bool    valid;
};

struct proc_table {
    struct proc_info *procs;
    int count;
    int capacity;
};

int  proc_table_init(struct proc_table *t);
void proc_table_free(struct proc_table *t);
int  proc_scan(struct proc_table *t);
int  proc_read_one(int pid, struct proc_info *out);
void proc_calc_cpu(struct proc_table *prev, struct proc_table *curr, double elapsed_sec);
void proc_sort_cpu(struct proc_table *t);
void proc_sort_mem(struct proc_table *t);
struct proc_info *proc_find(struct proc_table *t, int pid);

#endif