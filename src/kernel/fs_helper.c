#include "fs_helper.h"

struct proc_dir_entry *proc_root = 0;

static ssize_t dummy_proc_read(struct file *file,
                               char __user *buf,
                               size_t,
                               loff_t *off)
{
    return 0;
}

void proc_find_init(void)
{
    static struct proc_dir_entry *node;
    if (!(node = proc_create("node", 0644, 0,
                             &(struct proc_ops){.proc_read = dummy_proc_read})))
        return;

    proc_root = node->parent;
    proc_remove(node);
}
