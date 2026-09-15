#include <stdio.h>
#include <sys/stat.h>

int main(void)
{
    struct stat st;

    if (stat("README.md", &st) == -1) {
        perror("stat");
        return 1;
    }

    printf("Size: %ld bytes\n", st.st_size);

    return 0;
}