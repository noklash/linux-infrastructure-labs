#define _DEFAULT_SOURCE

#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

static void print_type(mode_t mode)
{
    if (S_ISREG(mode)) {
        printf("Type: regular file\n");
    } else if (S_ISDIR(mode)) {
        printf("Type: directory\n");
    } else if (S_ISLNK(mode)) {
        printf("Type: symbolic link\n");
    } else if (S_ISCHR(mode)) {
        printf("Type: character device\n");
    } else if (S_ISBLK(mode)) {
        printf("Type: block device\n");
    } else if (S_ISFIFO(mode)) {
        printf("Type: FIFO\n");
    } else if (S_ISSOCK(mode)) {
        printf("Type: socket\n");
    } else {
        printf("Type: unknown\n");
    }
}

int main(int argc, char *argv[])
{
    struct stat st;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s info <path>\n", argv[0]);
        return 1;
    }

    if (stat(argv[2], &st) == -1) {
        perror("stat");
        return 1;
    }

    printf("Device: %" PRIuMAX "\n", (uintmax_t)st.st_dev);
    printf("Inode: %" PRIuMAX "\n", (uintmax_t)st.st_ino);
    printf("Links: %" PRIuMAX "\n", (uintmax_t)st.st_nlink);
    printf("Size: %" PRIdMAX " bytes\n", (intmax_t)st.st_size);
    print_type(st.st_mode);

    return 0;
}