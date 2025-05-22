#include "devices.h"

static LIST_HEAD(chrdev_list);

static char *devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;
    return NULL;
}

static int create_dev(char *name,
                      int *pmajor,
                      struct class **pcls,
                      struct file_operations *ops)
{
    *pmajor = register_chrdev(0, name, ops);
    if (*pmajor < 0)
        return *pmajor;
    *pcls = class_create(name);
    (*pcls)->devnode = (void *) devnode;
    device_create((*pcls), NULL, MKDEV(*pmajor, 0), NULL, name);
    printk("device /dev/%s is created\n", name);
    return 0;
}

static void destroy_dev(char *name, int major, struct class *cls)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, name);
    printk("device /dev/%s is destroyed\n", name);
}

void chrdev_add(const char *name, struct file_operations *ops)
{
    struct chrdev *dev = (struct chrdev *) vmalloc(sizeof(struct chrdev));
    if (!dev)
        goto OUT;
    dev->name = vmalloc(strlen(name) + 1);
    if (!dev->name)
        goto RELEASE_DEV;

    strcpy(dev->name, name);
    if (!create_dev(dev->name, &dev->major, &dev->cls, ops)) {
        list_add(&dev->list, &chrdev_list);
        goto OUT;
    }
    vfree(dev->name);
RELEASE_DEV:
    vfree(dev);
OUT:
}

void chrdev_release(void)
{
    struct chrdev *now, *safe;
    list_for_each_entry_safe (now, safe, &chrdev_list, list) {
        destroy_dev(now->name, now->major, now->cls);
        list_del(&now->list);
        vfree(now->name);
        vfree(now);
    }
}
