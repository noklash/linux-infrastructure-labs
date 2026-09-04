#ifndef MEMMAP_PROC_H
#define MEMMAP_PROC_H

#include "memory.h"

int proc_read_comm(unsigned long pid, char **out_comm);
int collect_process_memory(unsigned long pid, struct memory_map *mm);

#endif