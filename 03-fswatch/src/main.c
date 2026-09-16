#include <stdio.h>
#include <sys/stat.h>
#include <inttypes.h>

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

    printf("Size: %" PRIdMAX " bytes\n", (intmax_t)st.st_size);

    if (S_ISREG(st.st_mode)) {
        printf("Type: regular file\n");
    } else if (S_ISDIR(st.st_mode)) {
        printf("Type: directory\n");
    } else if (S_ISLNK(st.st_mode)) {
        printf("Type: symbolic link\n");
    } else if (S_ISCHR(st.st_mode)) {
        printf("Type: character device\n");
    } else if (S_ISBLK(st.st_mode)) {
        printf("Type: block device\n");
    } else if (S_ISFIFO(st.st_mode)) {
        printf("Type: FIFO\n");
    } else if (S_ISSOCK(st.st_mode)) {
        printf("Type: socket\n");
    } else {
        printf("Type: unknown\n");
    }

    return 0;
}