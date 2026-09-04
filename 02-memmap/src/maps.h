#ifndef MEMMAP_MAPS_H
#define MEMMAP_MAPS_H

#include "memory.h"

int parse_maps_line(const char *line, struct memory_mapping *out);
int collect_maps(unsigned long pid, struct memory_map *mm);

#endif