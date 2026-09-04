#include "cli.h"
#include "util.h"
#include "memory.h"
#include "proc.h"
#include "analyzer.h"
#include "display.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static volatile int g_stop = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

static int run_once(const struct cli_options *opts)
{
    struct memory_map mm;

    memory_map_init(&mm);
    if (collect_process_memory(opts->pid, &mm) != 0) {
        memory_map_free(&mm);
        return 1;
    }

    classify_mappings(&mm);
    sort_mappings(&mm, opts->sort);

    if (opts->json)
        display_json(&mm, opts);
    else if (opts->summary)
        display_summary(&mm);
    else if (opts->libraries)
        display_libraries(&mm);
    else if (opts->heap)
        display_region(&mm, MAP_HEAP, "HEAP");
    else if (opts->stack)
        display_region(&mm, MAP_STACK, "STACK");
    else
        display_maps(&mm, opts);

    memory_map_free(&mm);
    return 0;
}

int main(int argc, char **argv)
{
    struct cli_options opts;

    if (!parse_cli(argc, argv, &opts))
        return 1;
    if (opts.help) {
        print_usage(argv[0]);
        return 0;
    }

    if (opts.watch) {
        signal(SIGINT, on_signal);
        signal(SIGTERM, on_signal);
        while (!g_stop) {
            if (run_once(&opts) != 0)
                break;
            printf("\n--- refresh %ds (Ctrl+C to stop) ---\n\n", opts.interval);
            sleep((unsigned)opts.interval);
        }
        return 0;
    }

    return run_once(&opts);
}