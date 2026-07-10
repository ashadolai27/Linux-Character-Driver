#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME     "mychardev"
#define CLASS_NAME      "mycharclass"
#define BUFFER_SIZE     1024

/* Kernel buffer */
static char kernel_buffer[BUFFER_SIZE];

/* Device variables */
static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

/*----------------------------------------------------------*/
/* File Operations */
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

static ssize_t driver_write(struct file *file,
                            const char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    size_t bytes_to_copy;

    bytes_to_copy = min(count, (size_t)(BUFFER_SIZE - 1));

    if (copy_from_user(kernel_buffer, buf, bytes_to_copy))
        return -EFAULT;

    kernel_buffer[bytes_to_copy] = '\0';

    pr_info("mychardev: Received %zu bytes: %s\n",
            bytes_to_copy, kernel_buffer);

    return bytes_to_copy;
}

static ssize_t driver_read(struct file *file,
                           char __user *buf,
                           size_t count,
                           loff_t *ppos)
{
    size_t data_len;

    data_len = strlen(kernel_buffer);

    if (*ppos >= data_len)
        return 0;

    if (count > data_len - *ppos)
        count = data_len - *ppos;

    if (copy_to_user(buf, kernel_buffer + *ppos, count))
        return -EFAULT;

    *ppos += count;

    pr_info("mychardev: Sent %zu bytes\n", count);

    return count;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = driver_open,
    .release = driver_release,
    .read    = driver_read,
    .write   = driver_write,
};

/*----------------------------------------------------------*/
/* Module Init */
/*----------------------------------------------------------*/

static int __init char_driver_init(void)
{
    int ret;

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    /* Initialize cdev */
    cdev_init(&my_cdev, &fops);

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret)
        goto unregister_region;

    /* Create class */
    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        goto del_cdev;
    }

    /* Create device node */
    if (IS_ERR(device_create(my_class,
                             NULL,
                             dev_num,
                             NULL,
                             DEVICE_NAME))) {
        ret = -ENOMEM;
        goto destroy_class;
    }

    pr_info("---------------------------------\n");
    pr_info("Character Driver Loaded\n");
    pr_info("Major Number : %d\n", MAJOR(dev_num));
    pr_info("Minor Number : %d\n", MINOR(dev_num));
    pr_info("Device : /dev/%s\n", DEVICE_NAME);
    pr_info("---------------------------------\n");

    return 0;

destroy_class:
    class_destroy(my_class);

del_cdev:
    cdev_del(&my_cdev);

unregister_region:
    unregister_chrdev_region(dev_num, 1);

    return ret;
}

/*----------------------------------------------------------*/
/* Module Exit */
/*----------------------------------------------------------*/

static void __exit char_driver_exit(void)
{
    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("Character Driver Unloaded\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Asha");
MODULE_DESCRIPTION("Simple Linux Character Driver");
MODULE_VERSION("1.0");
