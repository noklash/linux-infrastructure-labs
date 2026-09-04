#ifndef MEMMAP_MEMORY_H
#define MEMMAP_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    MAP_UNKNOWN = 0,
    MAP_EXECUTABLE,
    MAP_SHARED_LIBRARY,
    MAP_HEAP,
    MAP_STACK,
    MAP_VDSO,
    MAP_VVAR,
    MAP_VSYSCALL,
    MAP_ANONYMOUS,
    MAP_FILE_BACKED
} mapping_type_t;

struct mapping_stats {
    unsigned long size;
    unsigned long rss;
    unsigned long pss;
    unsigned long shared_clean;
    unsigned long shared_dirty;
    unsigned long private_clean;
    unsigned long private_dirty;
    unsigned long referenced;
    unsigned long anonymous;
    unsigned long swap;
    bool has_smaps;
};

struct memory_mapping {
    unsigned long start;
    unsigned long end;
    char perms[5];
    unsigned long offset;
    char device[32];
    unsigned long inode;
    char *pathname;

    unsigned long size;
    mapping_type_t type;
    struct mapping_stats stats;
};

struct memory_map {
    struct memory_mapping *items;
    size_t count;
    size_t capacity;
    unsigned long pid;
    char *comm;
    long page_size;
};

void memory_map_init(struct memory_map *mm);
void memory_map_free(struct memory_map *mm);
int  memory_map_push(struct memory_map *mm, const struct memory_mapping *m);
const char *mapping_type_str(mapping_type_t t);

#endif