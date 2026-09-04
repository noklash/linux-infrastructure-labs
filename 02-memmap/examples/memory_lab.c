#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(void)
{
    size_t i;
    char *heap;
    void *anon;
    int fd;

    printf("memory_lab PID: %d\n", getpid());
    printf("Phase A: malloc 32 MB (untouched)\n");
    heap = malloc(32UL * 1024 * 1024);
    if (!heap) {
        perror("malloc");
        return 1;
    }

    printf("Sleep 15s — try: memmap --summary %d\n", getpid());
    sleep(15);

    printf("Phase B: touch all heap pages\n");
    for (i = 0; i < 32UL * 1024 * 1024; i += 4096)
        heap[i] = 1;

    printf("Phase C: anonymous mmap 16 MB\n");
    anon = mmap(NULL, 16UL * 1024 * 1024, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (anon != MAP_FAILED)
        memset(anon, 0xab, 16UL * 1024 * 1024);

    printf("Phase D: file mmap /etc/passwd\n");
    fd = open("/etc/passwd", O_RDONLY);
    if (fd >= 0) {
        void *fm = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
        (void)fm;
    }

    printf("Sleeping 120s for inspection...\n");
    sleep(120);

    free(heap);
    if (anon != MAP_FAILED)
        munmap(anon, 16UL * 1024 * 1024);
    return 0;
}