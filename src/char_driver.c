#include "../include/mychardev_ioctl.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define DEVICE_NAME        "mychardev"
#define CLASS_NAME         "mycharclass"
#define INITIAL_CAPACITY   64

/* Dynamic buffer */
static char *kernel_buffer = NULL;
static size_t buffer_capacity = 0;
static size_t data_size = 0;

/* Device variables */
static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

/* Mutex */
static DEFINE_MUTEX(buffer_lock);

/*----------------------------------------------------------*/

static int driver_open(struct inode *inode, struct file *file)
{
    pr_info("mychardev: Device opened\n");
    return 0;
}

static int driver_release(struct inode *inode, struct file *file)
{
    pr_info("mychardev: Device closed\n");
    return 0;
}

/*----------------------------------------------------------*/

static size_t next_capacity(size_t needed)
{
    size_t cap;

    cap = max(buffer_capacity, (size_t)INITIAL_CAPACITY);

    while (cap < needed)
        cap <<= 1;

    return cap;
}

/*----------------------------------------------------------*/

static ssize_t driver_write(struct file *file,
                            const char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    ssize_t ret;
    char *temp;
    size_t new_capacity;

    if (!buf)
        return -EINVAL;

    if (count == 0)
        return 0;

    mutex_lock(&buffer_lock);

    if (count + 1 > buffer_capacity) {

        new_capacity = next_capacity(count + 1);

        temp = krealloc(kernel_buffer,
                        new_capacity,
                        GFP_KERNEL);

        if (!temp) {
            pr_err("mychardev: Memory allocation failed\n");
            ret = -ENOMEM;
            goto out;
        }

        kernel_buffer = temp;
        buffer_capacity = new_capacity;

        pr_info("mychardev: Buffer resized to %zu bytes\n",
                buffer_capacity);
    }

    if (copy_from_user(kernel_buffer, buf, count)) {
        pr_err("mychardev: copy_from_user failed\n");
        ret = -EFAULT;
        goto out;
    }

    kernel_buffer[count] = '\0';

    data_size = count;

    *ppos = 0;

    pr_info("mychardev: Received %zu bytes(Capacity:%zu)\n", data_size,buffer_capacity);

    ret = count;

out:
    mutex_unlock(&buffer_lock);
    return ret;
}

/*----------------------------------------------------------*/

static ssize_t driver_read(struct file *file,
                           char __user *buf,
                           size_t count,
                           loff_t *ppos)
{
    ssize_t ret;
    size_t bytes_to_copy;

    mutex_lock(&buffer_lock);

    if (!kernel_buffer) {
        ret = 0;
        goto out;
    }

    if (*ppos >= data_size) {
        ret = 0;
        goto out;
    }

    bytes_to_copy = min(count,
                        data_size - (size_t)*ppos);

    if (copy_to_user(buf,
                     kernel_buffer + *ppos,
                     bytes_to_copy)) {

        pr_err("mychardev: copy_to_user failed\n");
        ret = -EFAULT;
        goto out;
    }

    *ppos += bytes_to_copy;

    pr_info("mychardev: Sent %zu bytes (Offset:%lld)\n",
            bytes_to_copy,*ppos);

    ret = bytes_to_copy;

out:
    mutex_unlock(&buffer_lock);
    return ret;
}

/*----------------------------------------------------------*/
static long driver_ioctl(struct file *file,
                         unsigned int cmd,
                         unsigned long arg)
{
    long ret = 0;
    size_t new_size;
    char *temp;
    mutex_lock(&buffer_lock);

    switch (cmd)
    {
        case CLEAR_BUFFER:

            data_size = 0;

            if (kernel_buffer)
                kernel_buffer[0] = '\0';

            pr_info("mychardev: Buffer cleared\n");
            break;
	case GET_BUFFER_SIZE:

        if (copy_to_user((void __user *)arg,
                         &buffer_capacity,
                         sizeof(buffer_capacity)))
        {
            ret = -EFAULT;
            goto out;
        }

        pr_info("mychardev: Buffer Capacity = %zu\n",
                buffer_capacity);
        break;
	case GET_DATA_SIZE:

    if (copy_to_user((void __user *)arg,
                     &data_size,
                     sizeof(data_size)))
    {
        ret = -EFAULT;
        goto out;
    }

    pr_info("mychardev: Data Size = %zu\n",
            data_size);

    break;
    case RESIZE_BUFFER:

    if (copy_from_user(&new_size,
                       (void __user *)arg,
                       sizeof(new_size)))
    {
        ret = -EFAULT;
        goto out;
    }

    if (new_size == 0)
    {
        ret = -EINVAL;
        goto out;
    }

    temp = krealloc(kernel_buffer,
                    new_size,
                    GFP_KERNEL);

    if (!temp)
    {
        ret = -ENOMEM;
        goto out;
    }

    kernel_buffer = temp;
    buffer_capacity = new_size;

    if (data_size > buffer_capacity)
        data_size = buffer_capacity;

    pr_info("mychardev: Buffer resized to %zu bytes\n",
            buffer_capacity);

    break;

        default:
            ret = -EINVAL;
            pr_err("mychardev: Invalid ioctl command\n");
            break;
    }
    out:

    mutex_unlock(&buffer_lock);

    return ret;
}
/*---------------------------------------------------------*/

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = driver_open,
    .release = driver_release,
    .read    = driver_read,
    .write   = driver_write,
    .unlocked_ioctl = driver_ioctl,
};

/*----------------------------------------------------------*/

static int __init char_driver_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num,
                              0,
                              1,
                              DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&my_cdev, &fops);

    ret = cdev_add(&my_cdev,
                   dev_num,
                   1);
    if (ret)
        goto err_unregister;

    my_class = class_create(CLASS_NAME);

    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto err_cdev;
    }

    if (IS_ERR(device_create(my_class,
                             NULL,
                             dev_num,
                             NULL,
                             DEVICE_NAME))) {
        ret = -ENOMEM;
        goto err_class;
    }

    pr_info("Character Driver Loaded\n");
    pr_info("Major=%d Minor=%d\n",
            MAJOR(dev_num),
            MINOR(dev_num));

    return 0;

err_class:
    class_destroy(my_class);

err_cdev:
    cdev_del(&my_cdev);

err_unregister:
    unregister_chrdev_region(dev_num, 1);

    return ret;
}

/*----------------------------------------------------------*/

static void __exit char_driver_exit(void)
{
    kfree(kernel_buffer);

    kernel_buffer = NULL;
    buffer_capacity = 0;
    data_size = 0;

    device_destroy(my_class, dev_num);
    class_destroy(my_class);

    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num, 1);

    pr_info("Character Driver Unloaded\n");
}

/*----------------------------------------------------------*/

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asha");
MODULE_DESCRIPTION("Character Driver with Dynamic Buffer and Mutex");
MODULE_VERSION("1.2");
