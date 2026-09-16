#include <stdio.h>
#include <sys/stat.h>

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

    printf("Size: %ld bytes\n", st.st_size);

    if (S_ISREG(st.st_mode)) {
        printf("Type: regular file\n");
    } else if (S_ISDIR(st.st_mode)) {
        printf("Type: directory\n");
    } else {
        printf("Type: other\n");
    }

    return 0;
}