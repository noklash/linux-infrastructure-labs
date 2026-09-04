#include "cli.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parse_cli(int argc, char **argv, struct cli_options *opts)
{
    int i;

    if (!opts)
        return false;

    memset(opts, 0, sizeof(*opts));
    opts->interval = 1;

    if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            opts->help = true;
            opts->valid = true;
            return true;
        }
        if (!strcmp(arg, "--summary"))   { opts->summary = true; continue; }
        if (!strcmp(arg, "--libraries")) { opts->libraries = true; continue; }
        if (!strcmp(arg, "--heap"))      { opts->heap = true; continue; }
        if (!strcmp(arg, "--stack"))     { opts->stack = true; continue; }
        if (!strcmp(arg, "--exec"))      { opts->filter_exec = true; continue; }
        if (!strcmp(arg, "--write"))     { opts->filter_write = true; continue; }
        if (!strcmp(arg, "--shared"))    { opts->filter_shared = true; continue; }
        if (!strcmp(arg, "--private"))   { opts->filter_private = true; continue; }
        if (!strcmp(arg, "--anonymous")) { opts->filter_anonymous = true; continue; }
        if (!strcmp(arg, "--json"))      { opts->json = true; continue; }
        if (!strcmp(arg, "--watch"))     { opts->watch = true; continue; }

        if (!strcmp(arg, "--interval")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --interval needs a value\n");
                return false;
            }
            opts->interval = atoi(argv[++i]);
            if (opts->interval < 1)
                opts->interval = 1;
            continue;
        }

        if (!strcmp(arg, "--sort")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --sort needs size|rss|pss|start\n");
                return false;
            }
            i++;
            if (!strcmp(argv[i], "size"))       opts->sort = SORT_SIZE;
            else if (!strcmp(argv[i], "rss"))   opts->sort = SORT_RSS;
            else if (!strcmp(argv[i], "pss"))   opts->sort = SORT_PSS;
            else if (!strcmp(argv[i], "start")) opts->sort = SORT_START;
            else {
                fprintf(stderr, "Error: unknown sort key '%s'\n", argv[i]);
                return false;
            }
            continue;
        }

        if (arg[0] != '-') {
            if (opts->pid) {
                fprintf(stderr, "Error: multiple PID arguments\n");
                return false;
            }
            if (!parse_pid(arg, &opts->pid)) {
                fprintf(stderr, "Error: invalid PID '%s'\n", arg);
                return false;
            }
            continue;
        }

        fprintf(stderr, "Error: unknown option '%s'\n", arg);
        print_usage(argv[0]);
        return false;
    }

    if (!opts->pid && !opts->help) {
        fprintf(stderr, "Error: PID is required\n");
        print_usage(argv[0]);
        return false;
    }

    opts->valid = true;
    return true;
}