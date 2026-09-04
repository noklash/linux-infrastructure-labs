#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define ALLOCATION_SIZE (100 * 1024 * 1024)

int main(void)
{
    char *memory = mmap(
        NULL,
        ALLOCATION_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Allocated 100 MB of virtual memory\n");
    printf("PID: %d\n", getpid());
    printf("Touching memory...\n");

    for (size_t i = 0; i < ALLOCATION_SIZE; i += 4096) {
        memory[i] = 1;
    }

    printf("Memory touched.\n");
    printf("Press Ctrl+C to exit.\n");

    while (1) {
        sleep(1);
    }

    munmap(memory, ALLOCATION_SIZE);
    return 0;
}