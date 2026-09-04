#ifndef MEMMAP_ANALYZER_H
#define MEMMAP_ANALYZER_H

#include "memory.h"
#include "cli.h"

void classify_mappings(struct memory_map *mm);

void analyze_summary(const struct memory_map *mm,
                     unsigned long *vss, unsigned long *rss,
                     unsigned long *pss, unsigned long *anon,
                     unsigned long *file, unsigned long *exec_sz,
                     unsigned long *writable, unsigned long *shared,
                     unsigned long *priv);

bool mapping_matches_filters(const struct memory_mapping *m,
                             const struct cli_options *opts);

void sort_mappings(struct memory_map *mm, sort_key_t key);

#endif