#pragma once
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/vmalloc.h>

struct chrdev {
    char *name;
    int major;
    struct class *cls;
    struct list_head list;
};

/**
 * chrdev_add - Add a new character device with file operations
 *
 * @name: the name of device
 * @ops: the pointer to file operations structure
 */
void chrdev_add(const char *name, struct file_operations *ops);

/**
 * chrdev_release - Release all of the character devices
 */
void chrdev_release(void);
