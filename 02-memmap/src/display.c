#include "display.h"
#include "analyzer.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void display_maps(const struct memory_map *mm, const struct cli_options *opts)
{
    size_t i;
    char szbuf[32], rssbuf[32];

    printf("MEMORY MAP\n");
    printf("PID: %lu\n", mm->pid);
    if (mm->comm)
        printf("NAME: %s\n", mm->comm);
    printf("Page size: %ld bytes\n\n", mm->page_size);

    printf("%-16s %-16s %-8s %-6s %-10s %-10s %s\n",
           "START", "END", "SIZE", "PERMS", "RSS", "TYPE", "PATH");
    printf("---------------------------------------------------------------------------------------------\n");

    for (i = 0; i < mm->count; i++) {
        const struct memory_mapping *m = &mm->items[i];
        if (!mapping_matches_filters(m, opts))
            continue;

        format_size(m->size, szbuf, sizeof(szbuf));
        if (m->stats.has_smaps)
            format_size(m->stats.rss, rssbuf, sizeof(rssbuf));
        else
            snprintf(rssbuf, sizeof(rssbuf), "-");

        printf("%016lx %016lx %-8s %-6s %-10s %-10s %s\n",
               m->start, m->end, szbuf, m->perms, rssbuf,
               mapping_type_str(m->type),
               m->pathname ? m->pathname : "");
    }
}

void display_summary(const struct memory_map *mm)
{
    unsigned long vss, rss, pss, anon, file, exec_sz, writable, shared, priv;
    char b[32];

    analyze_summary(mm, &vss, &rss, &pss, &anon, &file, &exec_sz,
                    &writable, &shared, &priv);

    printf("MEMORY SUMMARY\n");
    printf("PID: %lu\n", mm->pid);
    if (mm->comm)
        printf("NAME: %s\n", mm->comm);
    printf("\n");

    format_size(vss, b, sizeof(b));      printf("Virtual Memory (mapped):  %s\n", b);
    format_size(rss, b, sizeof(b));      printf("Resident (RSS):           %s\n", b);
    format_size(pss, b, sizeof(b));      printf("Proportional (PSS):       %s\n", b);
    format_size(anon, b, sizeof(b));     printf("Anonymous (RSS):          %s\n", b);
    format_size(file, b, sizeof(b));     printf("File-backed (RSS):        %s\n", b);
    format_size(exec_sz, b, sizeof(b));  printf("Executable mappings:      %s\n", b);
    format_size(writable, b, sizeof(b)); printf("Writable mappings:        %s\n", b);
    format_size(shared, b, sizeof(b));   printf("Shared mappings:          %s\n", b);
    format_size(priv, b, sizeof(b));     printf("Private mappings:         %s\n", b);

    printf("\nNote: VSS is virtual address space, not physical RAM.\n");
    printf("RSS = resident pages; PSS shares cost of shared pages.\n");
}

void display_libraries(const struct memory_map *mm)
{
    size_t i;

    printf("SHARED LIBRARIES\n\n");
    printf("%-50s %s\n", "LIBRARY", "VIRTUAL SIZE");
    printf("------------------------------------------------------------\n");

    for (i = 0; i < mm->count; i++) {
        const struct memory_mapping *m = &mm->items[i];
        char b[32];

        if (m->type != MAP_SHARED_LIBRARY || !m->pathname)
            continue;
        if (!strchr(m->perms, 'x'))
            continue;

        format_size(m->size, b, sizeof(b));
        printf("%-50s %s\n", m->pathname, b);
    }
}

void display_region(const struct memory_map *mm, mapping_type_t type,
                    const char *title)
{
    size_t i;

    printf("%s\n\n", title);
    for (i = 0; i < mm->count; i++) {
        const struct memory_mapping *m = &mm->items[i];
        char b[32], r[32];

        if (m->type != type)
            continue;

        format_size(m->size, b, sizeof(b));
        format_size(m->stats.rss, r, sizeof(r));

        printf("  Address:     %016lx - %016lx\n", m->start, m->end);
        printf("  Size:        %s\n", b);
        printf("  Permissions: %s\n", m->perms);
        printf("  RSS:         %s\n", m->stats.has_smaps ? r : "n/a");
        printf("  Path:        %s\n\n", m->pathname ? m->pathname : "(none)");
    }
}

void display_json(const struct memory_map *mm, const struct cli_options *opts)
{
    size_t i;
    int first = 1;

    printf("{\n  \"pid\": %lu,\n", mm->pid);
    if (mm->comm)
        printf("  \"name\": \"%s\",\n", mm->comm);
    printf("  \"page_size\": %ld,\n", mm->page_size);
    printf("  \"mappings\": [\n");

    for (i = 0; i < mm->count; i++) {
        const struct memory_mapping *m = &mm->items[i];
        if (!mapping_matches_filters(m, opts))
            continue;
        if (!first)
            printf(",\n");
        first = 0;

        printf("    {\n");
        printf("      \"start\": \"0x%lx\",\n", m->start);
        printf("      \"end\": \"0x%lx\",\n", m->end);
        printf("      \"size\": %lu,\n", m->size);
        printf("      \"permissions\": \"%s\",\n", m->perms);
        printf("      \"offset\": %lu,\n", m->offset);
        printf("      \"inode\": %lu,\n", m->inode);
        printf("      \"path\": \"%s\",\n", m->pathname ? m->pathname : "");
        printf("      \"type\": \"%s\",\n", mapping_type_str(m->type));
        printf("      \"rss\": %lu,\n", m->stats.rss);
        printf("      \"pss\": %lu\n", m->stats.pss);
        printf("    }");
    }
    printf("\n  ]\n}\n");
}