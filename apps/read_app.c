#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd;

    char buffer[100];

    fd = open("/dev/mychardev", O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    read(fd, buffer, sizeof(buffer));

    printf("Kernel says : %s\n", buffer);

    close(fd);

    return 0;
}
