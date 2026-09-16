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

    return 0;
}