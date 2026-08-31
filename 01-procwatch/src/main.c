#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "proc.h"
#include "display.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  (no args)          list processes sorted by CPU\n"
        "  --pid PID          inspect a single process\n"
        "  --top cpu|memory   sort by CPU or memory (default cpu)\n"
        "  --tree             show process tree\n"
        "  --fds PID          list open file descriptors\n"
        "  --maps PID         show memory maps\n"
        "  --interval SEC     sampling interval for CPU%% (default 1.0)\n"
        "  --limit N          show at most N processes (default 40)\n"
        "  -h, --help         this help\n",
        prog);
}

int main(int argc, char **argv)
{
    int opt_pid = -1;
    int opt_fds = -1;
    int opt_maps = -1;
    int do_tree = 0;
    const char *sort_by = "cpu";
    double interval = 1.0;
    int limit = 40;

    static struct option longopts[] = {
        {"pid",      required_argument, 0, 'p'},
        {"top",      required_argument, 0, 't'},
        {"tree",     no_argument,       0, 'T'},
        {"fds",      required_argument, 0, 'f'},
        {"maps",     required_argument, 0, 'm'},
        {"interval", required_argument, 0, 'i'},
        {"limit",    required_argument, 0, 'l'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int c;

    while ((c = getopt_long(argc, argv, "p:t:Tf:m:i:l:h", longopts, NULL)) != -1) {
        switch (c) {
        case 'p':
            opt_pid = atoi(optarg);
            break;

        case 't':
            sort_by = optarg;
            break;

        case 'T':
            do_tree = 1;
            break;

        case 'f':
            opt_fds = atoi(optarg);
            break;

        case 'm':
            opt_maps = atoi(optarg);
            break;

        case 'i':
            interval = atof(optarg);
            if (interval < 0.1)
                interval = 0.1;
            break;

        case 'l':
            limit = atoi(optarg);
            break;

        case 'h':
            usage(argv[0]);
            return 0;

        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (opt_fds > 0) {
        display_fds(opt_fds);
        return 0;
    }

    if (opt_maps > 0) {
        display_maps(opt_maps);
        return 0;
    }

    struct proc_table prev, curr;

    if (proc_table_init(&prev) < 0 || proc_table_init(&curr) < 0)
        die("out of memory");

    if (opt_pid > 0) {
        /* single process: two samples for CPU% */
        struct proc_info a, b;

        if (proc_read_one(opt_pid, &a) < 0)
            die("cannot read pid %d", opt_pid);

        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);

        struct timespec req;
        req.tv_sec = (time_t)interval;
        req.tv_nsec = (long)((interval - req.tv_sec) * 1e9);

        nanosleep(&req, NULL);

        clock_gettime(CLOCK_MONOTONIC, &t1);

        if (proc_read_one(opt_pid, &b) < 0)
            die("process %d disappeared", opt_pid);

        double elapsed = timeval_diff_sec(&t0, &t1);
        long tck = get_clk_tck();

        unsigned long long delta =
            (b.utime + b.stime) - (a.utime + a.stime);

        b.cpu_pct = 100.0 * ((double)delta / tck) / elapsed;

        display_one(&b);

        proc_table_free(&prev);
        proc_table_free(&curr);

        return 0;
    }

    /* full scan */
    if (proc_scan(&prev) < 0)
        die("initial scan failed");

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    struct timespec req;
    req.tv_sec = (time_t)interval;
    req.tv_nsec = (long)((interval - req.tv_sec) * 1e9);

    nanosleep(&req, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    if (proc_scan(&curr) < 0)
        die("second scan failed");

    double elapsed = timeval_diff_sec(&t0, &t1);

    proc_calc_cpu(&prev, &curr, elapsed);

    if (do_tree) {
        display_tree(&curr);
    } else {
        if (strcmp(sort_by, "memory") == 0 ||
            strcmp(sort_by, "mem") == 0) {
            proc_sort_mem(&curr);
        } else {
            proc_sort_cpu(&curr);
        }

        display_table(&curr, limit);
    }

    proc_table_free(&prev);
    proc_table_free(&curr);

    return 0;
}