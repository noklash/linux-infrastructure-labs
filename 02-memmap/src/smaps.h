#ifndef MEMMAP_SMAPS_H
#define MEMMAP_SMAPS_H

#include "memory.h"

int collect_smaps(unsigned long pid, struct memory_map *mm);

#endif