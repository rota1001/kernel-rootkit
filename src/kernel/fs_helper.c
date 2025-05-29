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



static int cmp(char *x, char *y)
{
    if (strlen(x) != strlen(y))
        return (int) strlen(x) - (int) strlen(y);
    return strcmp(x, y);
}

/**
 * proc_find_child - Find the child with the specific name
 * @parent: the proc_dir_entry structure of parent
 * @s: string would be inserted
 *
 * Return: return a pointer to the child structure of NULL if not found
 */
static struct proc_dir_entry *proc_find_child(struct proc_dir_entry *parent,
                                              char *name)
{
    if (!parent || !name)
        return NULL;

    struct rb_node *node = parent->subdir.rb_node;

    while (node) {
        int k = cmp(
            name, container_of(node, struct proc_dir_entry, subdir_node)->name);
        if (k < 0)
            node = node->rb_left;
        else if (k > 0)
            node = node->rb_right;
        else
            return container_of(node, struct proc_dir_entry, subdir_node);
    }

    return NULL;
}

static char buf[PATH_MAX];

static struct proc_dir_entry *__proc_find_from_fix_point(
    struct proc_dir_entry *fix,
    char *pos)
{
    char *token;
    while ((token = strsep(&pos, "/")) != NULL) {
        if (!strcmp(token, ""))
            continue;
        printk("%s\n", token);
        fix = proc_find_child(fix, token);
        if (!fix)
            break;
    }
    return fix;
}

struct proc_dir_entry *proc_find_by_path(const char *path)
{
    if (strncmp(path, "/proc", 5) || strlen(path) > PATH_MAX)
        return NULL;

    strncpy(buf, path, PATH_MAX);
    char *pos = buf + 5;
    struct proc_dir_entry *now = proc_root;
    if (!strncmp(pos, "/net", 4)) {
        struct net *net;
        for_each_net(net)
        {
            strncpy(buf, path, PATH_MAX);
            struct proc_dir_entry *res =
                __proc_find_from_fix_point(net->proc_net, pos + 4);
            if (res)
                return res;
        }
        return NULL;
    }
    return __proc_find_from_fix_point(proc_root, pos);


    return now;
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