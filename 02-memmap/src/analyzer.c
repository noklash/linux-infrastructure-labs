#include "analyzer.h"

#include <string.h>
#include <stdlib.h>

static int is_so(const char *path)
{
    if (!path)
        return 0;
    return strstr(path, ".so") != NULL;
}

void classify_mappings(struct memory_map *mm)
{
    size_t i;
    const char *exe = NULL;

    for (i = 0; i < mm->count; i++) {
        struct memory_mapping *m = &mm->items[i];
        const char *p = m->pathname;

        if (p && !strcmp(p, "[heap]"))
            m->type = MAP_HEAP;
        else if (p && !strncmp(p, "[stack", 6))
            m->type = MAP_STACK;
        else if (p && !strcmp(p, "[vdso]"))
            m->type = MAP_VDSO;
        else if (p && !strcmp(p, "[vvar]"))
            m->type = MAP_VVAR;
        else if (p && !strcmp(p, "[vsyscall]"))
            m->type = MAP_VSYSCALL;
        else if (!p || !p[0])
            m->type = MAP_ANONYMOUS;
        else if (is_so(p) || strstr(p, "ld-linux") || strstr(p, "ld-musl"))
            m->type = MAP_SHARED_LIBRARY;
        else if (strchr(m->perms, 'x') && p[0] == '/') {
            m->type = MAP_EXECUTABLE;
            if (!exe)
                exe = p;
        } else if (p[0] == '/')
            m->type = MAP_FILE_BACKED;
        else
            m->type = MAP_UNKNOWN;
    }

    if (exe) {
        for (i = 0; i < mm->count; i++) {
            struct memory_mapping *m = &mm->items[i];
            if (m->pathname && !strcmp(m->pathname, exe))
                m->type = MAP_EXECUTABLE;
        }
    }
}

void analyze_summary(const struct memory_map *mm,
                     unsigned long *vss, unsigned long *rss,
                     unsigned long *pss, unsigned long *anon,
                     unsigned long *file, unsigned long *exec_sz,
                     unsigned long *writable, unsigned long *shared,
                     unsigned long *priv)
{
    size_t i;

    *vss = *rss = *pss = *anon = *file = *exec_sz = *writable = *shared = *priv = 0;

    for (i = 0; i < mm->count; i++) {
        const struct memory_mapping *m = &mm->items[i];
        unsigned long sz = m->size;
        unsigned long r = m->stats.has_smaps ? m->stats.rss : 0;
        unsigned long p = m->stats.has_smaps ? m->stats.pss : 0;

        *vss += sz;
        *rss += r;
        *pss += p;

        if (m->type == MAP_ANONYMOUS || m->type == MAP_HEAP || m->type == MAP_STACK)
            *anon += r ? r : 0;
        else if (m->pathname)
            *file += r ? r : 0;

        if (strchr(m->perms, 'x'))
            *exec_sz += sz;
        if (strchr(m->perms, 'w'))
            *writable += sz;
        if (strchr(m->perms, 's'))
            *shared += sz;
        else
            *priv += sz;
    }
}

bool mapping_matches_filters(const struct memory_mapping *m,
                             const struct cli_options *opts)
{
    if (opts->libraries && m->type != MAP_SHARED_LIBRARY)
        return false;
    if (opts->heap && m->type != MAP_HEAP)
        return false;
    if (opts->stack && m->type != MAP_STACK)
        return false;
    if (opts->filter_exec && !strchr(m->perms, 'x'))
        return false;
    if (opts->filter_write && !strchr(m->perms, 'w'))
        return false;
    if (opts->filter_shared && !strchr(m->perms, 's'))
        return false;
    if (opts->filter_private && !strchr(m->perms, 'p'))
        return false;
    if (opts->filter_anonymous &&
        m->type != MAP_ANONYMOUS && m->type != MAP_HEAP && m->type != MAP_STACK)
        return false;
    return true;
}

static int cmp_size(const void *a, const void *b)
{
    const struct memory_mapping *ma = a, *mb = b;
    if (ma->size < mb->size) return 1;
    if (ma->size > mb->size) return -1;
    return 0;
}

static int cmp_rss(const void *a, const void *b)
{
    unsigned long ra = ((const struct memory_mapping *)a)->stats.rss;
    unsigned long rb = ((const struct memory_mapping *)b)->stats.rss;
    if (ra < rb) return 1;
    if (ra > rb) return -1;
    return 0;
}

static int cmp_pss(const void *a, const void *b)
{
    unsigned long pa = ((const struct memory_mapping *)a)->stats.pss;
    unsigned long pb = ((const struct memory_mapping *)b)->stats.pss;
    if (pa < pb) return 1;
    if (pa > pb) return -1;
    return 0;
}

static int cmp_start(const void *a, const void *b)
{
    unsigned long sa = ((const struct memory_mapping *)a)->start;
    unsigned long sb = ((const struct memory_mapping *)b)->start;
    if (sa < sb) return -1;
    if (sa > sb) return 1;
    return 0;
}

void sort_mappings(struct memory_map *mm, sort_key_t key)
{
    if (key == SORT_NONE || mm->count < 2)
        return;
    switch (key) {
    case SORT_SIZE:  qsort(mm->items, mm->count, sizeof(*mm->items), cmp_size);  break;
    case SORT_RSS:   qsort(mm->items, mm->count, sizeof(*mm->items), cmp_rss);   break;
    case SORT_PSS:   qsort(mm->items, mm->count, sizeof(*mm->items), cmp_pss);   break;
    case SORT_START: qsort(mm->items, mm->count, sizeof(*mm->items), cmp_start); break;
    default: break;
    }
}