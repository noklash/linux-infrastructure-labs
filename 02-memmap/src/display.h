#ifndef MEMMAP_DISPLAY_H
#define MEMMAP_DISPLAY_H

#include "memory.h"
#include "cli.h"

void display_maps(const struct memory_map *mm, const struct cli_options *opts);
void display_summary(const struct memory_map *mm);
void display_json(const struct memory_map *mm, const struct cli_options *opts);
void display_libraries(const struct memory_map *mm);
void display_region(const struct memory_map *mm, mapping_type_t type, const char *title);

#endif