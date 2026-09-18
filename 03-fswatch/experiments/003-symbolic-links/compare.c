#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void print_stat(const char *label, const struct stat *st)
{
    printf("%s\n", label);
    printf("  device: %lu\n", (unsigned long)st->st_dev);
    printf("  inode:  %lu\n", (unsigned long)st->st_ino);
    printf("  links:  %lu\n", (unsigned long)st->st_nlink);
    printf("  size:   %ld bytes\n", (long)st->st_size);

    if (S_ISREG(st->st_mode)) {
        printf("  type:   regular file\n");
    } else if (S_ISDIR(st->st_mode)) {
        printf("  type:   directory\n");
    } else if (S_ISLNK(st->st_mode)) {
        printf("  type:   symbolic link\n");
    } else {
        printf("  type:   other\n");
    }

    printf("\n");
}

static void run_stat(const char *path)
{
    struct stat st;

    printf("=== stat() ===\n");

    if (stat(path, &st) == -1) {
        printf("  failed: %s\n", strerror(errno));
        printf("\n");
        return;
    }

    print_stat("  result:", &st);
}

static void run_lstat(const char *path)
{
    struct stat st;

    printf("=== lstat() ===\n");

    if (lstat(path, &st) == -1) {
        printf("  failed: %s\n", strerror(errno));
        printf("\n");
        return;
    }

    print_stat("  result:", &st);
}

static void run_readlink(const char *path)
{
    char buffer[256];
    ssize_t length;

    printf("=== readlink() ===\n");

    length = readlink(path, buffer, sizeof(buffer) - 1);

    if (length == -1) {
        printf("  failed: %s\n", strerror(errno));
        printf("\n");
        return;
    }

    buffer[length] = '\0';

    printf("  target: %s\n", buffer);
    printf("  length: %zd bytes\n", length);
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];

    printf("Path: %s\n\n", path);

    run_stat(path);
    run_lstat(path);
    run_readlink(path);

    return EXIT_SUCCESS;
}