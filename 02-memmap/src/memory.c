#include "memory.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

void memory_map_init(struct memory_map *mm)
{
    memset(mm, 0, sizeof(*mm));
    mm->page_size = get_page_size();
}

void memory_map_free(struct memory_map *mm)
{
    size_t i;

    if (!mm)
        return;
    for (i = 0; i < mm->count; i++)
        free(mm->items[i].pathname);
    free(mm->items);
    free(mm->comm);
    memset(mm, 0, sizeof(*mm));
}

int memory_map_push(struct memory_map *mm, const struct memory_mapping *m)
{
    if (mm->count == mm->capacity) {
        size_t nc = mm->capacity ? mm->capacity * 2 : 64;
        struct memory_mapping *ni = realloc(mm->items, nc * sizeof(*ni));
        if (!ni)
            return -1;
        mm->items = ni;
        mm->capacity = nc;
    }
    mm->items[mm->count] = *m;
    mm->count++;
    return 0;
}

const char *mapping_type_str(mapping_type_t t)
{
    switch (t) {
    case MAP_EXECUTABLE:     return "executable";
    case MAP_SHARED_LIBRARY: return "library";
    case MAP_HEAP:           return "heap";
    case MAP_STACK:          return "stack";
    case MAP_VDSO:           return "vdso";
    case MAP_VVAR:           return "vvar";
    case MAP_VSYSCALL:       return "vsyscall";
    case MAP_ANONYMOUS:      return "anonymous";
    case MAP_FILE_BACKED:    return "file";
    default:                 return "unknown";
    }
}