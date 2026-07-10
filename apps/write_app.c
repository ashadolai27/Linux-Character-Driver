#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd;

    char message[] = "Hello Kernel!";

    fd = open("/dev/mychardev", O_WRONLY);

    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    write(fd, message, strlen(message));

    close(fd);

    return 0;
}
