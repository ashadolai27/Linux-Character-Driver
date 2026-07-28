#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../include/mychardev_ioctl.h"

int main(void)
{
    int fd;
    size_t value;

    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Get current buffer capacity */
    if (ioctl(fd, GET_BUFFER_SIZE, &value) == -1)
    {
        perror("GET_BUFFER_SIZE");
    }
    else
    {
        printf("Buffer Capacity : %zu bytes\n", value);
    }

    /* Get current data size */
    if (ioctl(fd, GET_DATA_SIZE, &value) == -1)
    {
        perror("GET_DATA_SIZE");
    }
    else
    {
        printf("Data Size : %zu bytes\n", value);
    }

    /* Resize buffer */
    value = 256;

    if (ioctl(fd, RESIZE_BUFFER, &value) == -1)
    {
        perror("RESIZE_BUFFER");
    }
    else
    {
        printf("Buffer resized to %zu bytes\n", value);
    }

    /* Clear buffer */
    if (ioctl(fd, CLEAR_BUFFER) == -1)
    {
        perror("CLEAR_BUFFER");
    }
    else
    {
        printf("Buffer cleared successfully\n");
    }

    close(fd);

    return EXIT_SUCCESS;
}
