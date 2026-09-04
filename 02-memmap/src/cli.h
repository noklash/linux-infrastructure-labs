#ifndef MEMMAP_CLI_H
#define MEMMAP_CLI_H

#include <stdbool.h>

typedef enum {
    SORT_NONE = 0,
    SORT_SIZE,
    SORT_RSS,
    SORT_PSS,
    SORT_START
} sort_key_t;

struct cli_options {
    unsigned long pid;
    bool help;
    bool valid;

    bool summary;
    bool libraries;
    bool heap;
    bool stack;
    bool filter_exec;
    bool filter_write;
    bool filter_shared;
    bool filter_private;
    bool filter_anonymous;
    bool json;
    bool watch;
    int interval;
    sort_key_t sort;
};

bool parse_cli(int argc, char **argv, struct cli_options *opts);

#endif