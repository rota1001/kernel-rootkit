#include "fs_helper.h"

static inline struct proc_inode *PROC_I(const struct inode *inode)
{
    return container_of(inode, struct proc_inode, vfs_inode);
}

static inline struct proc_dir_entry *PDE(const struct inode *inode)
{
    return PROC_I(inode)->pde;
}


struct proc_dir_entry *proc_find_by_path(const char *path)
{
    if (strncmp(path, "/proc", 5) || strlen(path) > PATH_MAX)
        return NULL;

    struct file *filp = filp_open(path, O_RDONLY, 0);
    struct proc_dir_entry *pde = NULL;
    if (IS_ERR(filp))
        goto OUT;
    struct inode *inode = file_inode(filp);
    if (IS_ERR(inode))
        goto CLOSE;

    pde = PDE(inode);

CLOSE:
    filp_close(filp, 0);
OUT:
    return pde;
}

struct proc_ops *proc_get_proc_ops_by_path(const char *path)
{
    struct proc_dir_entry *res = proc_find_by_path(path);
    return (struct proc_ops *) (res ? res->proc_ops : 0);
}

struct seq_operations *proc_get_seq_ops_by_path(const char *path)
{
    struct proc_dir_entry *res = proc_find_by_path(path);
    return (struct seq_operations *) (res ? res->seq_ops : 0);
}

static void dfs(struct rb_node *node)
{
    if (!node)
        return;
    dfs(node->rb_left);
    printk("proc_list_dir: %s\n",
           container_of(node, struct proc_dir_entry, subdir_node)->name);
    dfs(node->rb_right);
}

void proc_list_dir(struct proc_dir_entry *entry)
{
    if (!entry)
        return;
    dfs(entry->subdir.rb_node);
}