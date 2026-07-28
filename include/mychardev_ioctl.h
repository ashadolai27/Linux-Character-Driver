#ifndef MYCHARDEV_IOCTL_H
#define MYCHARDEV_IOCTL_H

#include <linux/ioctl.h>

#define MYCHAR_MAGIC 'M'

#define CLEAR_BUFFER     _IO(MYCHAR_MAGIC, 0)
#define GET_BUFFER_SIZE  _IOR(MYCHAR_MAGIC, 1, size_t)
#define GET_DATA_SIZE    _IOR(MYCHAR_MAGIC, 2, size_t)
#define RESIZE_BUFFER    _IOW(MYCHAR_MAGIC, 3, size_t)

#endif
